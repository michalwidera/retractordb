#!/bin/bash
# Uruchamiane NA maszynie workera (bezposrednio, lub przez SSH z start_supervisor.sh).
# Realizuje pojedyncze badanie: snapshot stanu -> algorytm QRS (rec205-detect.rql) przy
# zadanej czestosci i liczbie klientow xqry -> zbior metryk systemowych -> snapshot stanu
# -> zapis wynikow do repo -> commit --amend + push --force-with-lease.
#
# Wymagania spelniane tutaj:
#  - dane zrodlowe i wyjscia procesu leza wylacznie w pamieci (pkt 17): /dev/shm, NIE /tmp
#    -- na czesci sprzetu (potwierdzone na Pi 400: /tmp to zwykly ext4 na karcie SD,
#    /dev/mmcblk0p2, nie tmpfs) /tmp wcale nie jest RAM-backed, wiec uzycie /tmp
#    zamiast realnego tmpfs pisaloby cichutko na karte SD wbrew wymaganiu
#  - RDB_BENCH_CSV jest jawnie ustawiane na sciezke w /dev/shm (probe nie pisze
#    domyslnie nigdzie -- patrz src/retractor/lib/executorsm.cpp:614)
#  - xqry nie zapisuje na karte SD (pkt 18): wyjscie idzie do /dev/null albo, na zadanie,
#    przez netcat na loopback
#
# Usage:
#   run_study.sh <branch> <campaign> <study_id> <rate_hz> <clients> <samples> [sink: null|nc]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
# shellcheck source=../lib/common.sh
source "$REPO_ROOT/examples/experiment/lib/common.sh"

[ $# -ge 6 ] || die "Usage: $0 <branch> <campaign> <study_id> <rate_hz> <clients> <samples> [sink]"
BRANCH="$1" CAMPAIGN="$2" STUDY_ID="$3" RATE_HZ="$4" CLIENTS="$5" SAMPLES="$6"
SINK="${7:-null}"

command -v xretractor >/dev/null || die "xretractor nie jest na PATH (zbuduj: scripts/buildrdb.sh conan ninja probe && ninja install w build/Release)"
command -v xqry >/dev/null       || die "xqry nie jest na PATH"

ECG_SRC="$REPO_ROOT/examples/ecg/rec205"
[ -f "$ECG_SRC/rec205" ] || die "Brak $ECG_SRC/rec205 -- uruchom examples/ecg/build.sh na tym repo najpierw"

[ -d /dev/shm ] || die "/dev/shm niedostepny na tej maszynie -- nie ma tmpfs do bezpiecznego uruchomienia badania"
WORKDIR="/dev/shm/rdb-experiment/study_${STUDY_ID}"
RESULTS_DIR="$REPO_ROOT/examples/experiment/results/${CAMPAIGN}/study_$(printf '%02d' "$STUDY_ID")"
STREAM_NAME="detect_out"
NC_PORT=$((20000 + STUDY_ID))

CLIENT_PIDS=()
METRICS_PID=""
XRETRACTOR_PID=""
NC_PID=""
ORIG_GOVERNOR=""

# Kontrola srodowiska pomiarowego:
#  - governor CPU pinowany na 'performance' na czas badania. Odkryto (JOURNAL.md,
#    2026-07-16), ze pod 'ondemand' czestotliwosc plywa 67-94% max i mediana E1
#    ROSNIE przy niskich czestosciach naplywu (wiecej uspien -> nizsze takty);
#    stosunek median 360Hz/1080Hz = 1,51 ~= 1,8GHz/1,2GHz. Bez pinowania governor
#    jest niekontrolowana zmienna zaklocajaca pomiar.
#  - xretractor przypinany do rdzenia XR_CPU (taskset), klienci xqry i sampler
#    metryk na pozostalych rdzeniach -- separacja procesu mierzonego od tla.
#    Pelna wylacznosc rdzenia wymaga isolcpus w cmdline jadra (osobna decyzja).
XR_CPU=3
BG_CPUS="0-2"

cleanup() {
  for pid in "${CLIENT_PIDS[@]:-}"; do [ -n "$pid" ] && kill "$pid" 2>/dev/null || true; done
  [ -n "$METRICS_PID" ] && kill "$METRICS_PID" 2>/dev/null || true
  [ -n "$XRETRACTOR_PID" ] && kill "$XRETRACTOR_PID" 2>/dev/null || true
  [ -n "$NC_PID" ] && kill "$NC_PID" 2>/dev/null || true
  # przywroc pierwotny governor takze przy awaryjnym wyjsciu
  if [ -n "$ORIG_GOVERNOR" ]; then
    echo "$ORIG_GOVERNOR" | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null 2>&1 || true
    ORIG_GOVERNOR=""
  fi
  wait 2>/dev/null || true
}
trap cleanup EXIT

set_performance_governor() {
  local gov_file=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  [ -r "$gov_file" ] || { log "UWAGA: brak cpufreq w sysfs -- pomijam pinowanie governora"; return; }
  if sudo -n true 2>/dev/null; then
    ORIG_GOVERNOR=$(cat "$gov_file")
    echo performance | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null
    log "Governor CPU: performance na czas badania (bylo: $ORIG_GOVERNOR)"
  else
    log "UWAGA: brak bezhaslowego sudo -- governor pozostaje $(cat "$gov_file"); pomiar podatny na skalowanie czestotliwosci!"
  fi
}

log "=== Badanie: campaign=$CAMPAIGN study=$STUDY_ID rate=${RATE_HZ}Hz clients=$CLIENTS samples=$SAMPLES sink=$SINK ==="

log "Weryfikacja/aktualizacja brancha $BRANCH na workerze..."
cd "$REPO_ROOT"
git fetch origin "$BRANCH"
git checkout "$BRANCH"
git reset --hard "origin/$BRANCH"

rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"
mkdir -p "$RESULTS_DIR"

# --- Dane zrodlowe wylacznie w /dev/shm (pkt 17/18) -----------------------------
cp "$ECG_SRC/rec205" "$ECG_SRC/rec205.desc" "$ECG_SRC/bp_coef.txt" "$ECG_SRC/d_coef.txt" "$WORKDIR/"
sed "s#1/360#1/${RATE_HZ}#" "$ECG_SRC/rec205-detect.rql" > "$WORKDIR/study.rql"

snapshot_state() {
  local label="$1" out="$2"
  {
    echo "# Stan maszyny -- $label"
    echo
    echo "- data: $(date -Is)"
    echo "- badanie: campaign=$CAMPAIGN study_id=$STUDY_ID rate_hz=$RATE_HZ clients=$CLIENTS samples=$SAMPLES"
    echo
    echo '## uname'
    echo '```'; uname -a; echo '```'
    echo '## dystrybucja'
    echo '```'; cat /etc/os-release 2>/dev/null || echo "brak /etc/os-release"; echo '```'
    echo '## CPU'
    echo '```'; lscpu 2>/dev/null || cat /proc/cpuinfo; echo '```'
    echo '## Pamiec'
    echo '```'; free -h; echo '```'
    echo '## Fragmentacja pamieci (buddyinfo)'
    echo '```'; cat /proc/buddyinfo 2>/dev/null || echo "niedostepne"; echo '```'
    echo '## Load average'
    echo '```'; cat /proc/loadavg; echo '```'
    echo '## Temperatura (strefy termiczne)'
    echo '```'
    for z in /sys/class/thermal/thermal_zone*/temp; do
      [ -r "$z" ] || continue
      printf '%s: %d m°C\n' "$z" "$(cat "$z")"
    done
    echo '```'
    echo '## Kernel cmdline (izolacja rdzeni itp.)'
    echo '```'; cat /proc/cmdline; echo '```'
    echo '## Governor i czestotliwosci CPU (chwilowe, per rdzen)'
    echo '```'
    for c in /sys/devices/system/cpu/cpu[0-9]*/cpufreq; do
      [ -r "$c/scaling_governor" ] || continue
      printf '%s: governor=%s cur=%s kHz min=%s max=%s kHz\n' \
        "$(basename "$(dirname "$c")")" "$(cat "$c/scaling_governor")" \
        "$(cat "$c/scaling_cur_freq")" "$(cat "$c/scaling_min_freq")" "$(cat "$c/scaling_max_freq")"
    done
    command -v vcgencmd >/dev/null && echo "throttled: $(vcgencmd get_throttled)"
    echo '```'
  } > "$out"
}

set_performance_governor
snapshot_state "PRZED badaniem" "$WORKDIR/state_before.md"

# --- Sampler CPU/pamiec/temperatura w tle -----------------------------------
sample_metrics() {
  local out="$1"
  echo "ts_epoch,load1,mem_used_kb,mem_avail_kb,temp_mC" > "$out"
  while true; do
    local ts load1 mem_used mem_avail temp
    ts=$(date +%s)
    load1=$(cut -d' ' -f1 /proc/loadavg)
    mem_used=$(awk '/MemTotal/{t=$2} /MemAvailable/{a=$2} END{print t-a}' /proc/meminfo)
    mem_avail=$(awk '/MemAvailable/{print $2}' /proc/meminfo)
    temp=""
    for z in /sys/class/thermal/thermal_zone*/temp; do
      [ -r "$z" ] && { temp=$(cat "$z"); break; }
    done
    echo "${ts},${load1},${mem_used},${mem_avail},${temp:-NA}" >> "$out"
    sleep 1
  done
}
# PIN_BG/PIN_XR puste, gdy taskset niedostepny (wtedy scheduler decyduje sam)
PIN_XR=""
PIN_BG=""
if command -v taskset >/dev/null; then
  PIN_XR="taskset -c $XR_CPU"
  PIN_BG="taskset -c $BG_CPUS"
else
  log "UWAGA: brak taskset -- procesy bez przypiecia do rdzeni"
fi

# sample_metrics to funkcja shella -- przypinamy subshell (taskset -p), nie exec
(
  [ -n "$PIN_BG" ] && taskset -p -c "$BG_CPUS" $BASHPID >/dev/null 2>&1
  sample_metrics "$WORKDIR/metrics.csv"
) &
METRICS_PID=$!

# --- Ujscie danych klientow xqry: /dev/null albo netcat na loopback --------
if [ "$SINK" = "nc" ] && command -v nc >/dev/null; then
  nc -lk 127.0.0.1 "$NC_PORT" > "$WORKDIR/nc_sink.log" 2>&1 &
  NC_PID=$!
  sleep 0.3
  if ! kill -0 "$NC_PID" 2>/dev/null; then
    log "nc -lk niedostepne na tym systemie, przechodze na sink=null"
    SINK="null"; NC_PID=""
  fi
else
  SINK="null"
fi

# --- Uruchomienie xretractor z sonda E1/E2E (RDB_BENCH_CSV -> /dev/shm) --------
cd "$WORKDIR"
# Bez -t (realtime) petla xretractor spi wzglednie (std::this_thread::sleep_for
# stalego okresu, nie absolutne budzenie) -- gdy compute_ns przekracza budzet
# interwalu (co jest oczekiwane wlasnie przy szukaniu granicy wydolnosci
# sprzetu, pkt 13), dryf narasta bez ograniczen i czas rzeczywisty realizacji
# NIE zbiega do samples/rate_hz. Budzet liczymy wiec od pesymistycznego czasu
# na probke (30ms, z zapasem ponad obserwowane ~13ms/iter na Pi 400),
# celowo NIEZALEZNIE od rate_hz -- przy wyzszych syntetycznych czestosciach
# ten efekt tylko sie poglebia, wiec nominalny okres nie jest miarodajny.
RUN_TIMEOUT=$(( SAMPLES * 30 / 1000 + 120 ))

# Tryb czasu rzeczywistego (-t): SCHED_FIFO + mlockall + absolutne budzenie.
# Wymaga capabilities na binarce (znikaja po kazdym rebuild/install -> nadajemy
# idempotentnie). Sciezka capabilities zamiast sudo-root: proces biegnie jako
# michal, wiec IPC shm i pliki wynikowe nie zmieniaja wlasciciela, a xqry
# lączy się bez dodatkowych uprawnien. Brak setcap = miekka degradacja
# (rtCheckAndPrint odmowi aktywacji RT, badanie pobiegnie jak dotychczas).
# RT throttling jadra zostaje na domyslnych 95% jako zawor bezpieczenstwa --
# przy czestosciach, gdzie compute > budzet, watek FIFO liczy niemal ciagle
# i pelne wylaczenie throttlingu mogloby zaglodzic rdzen.
XR_BIN="$(command -v xretractor)"
if command -v getcap >/dev/null && ! getcap "$XR_BIN" 2>/dev/null | grep -q "cap_sys_nice"; then
  if sudo -n setcap cap_sys_nice,cap_ipc_lock+ep "$XR_BIN" 2>/dev/null; then
    log "Nadano capabilities RT (cap_sys_nice,cap_ipc_lock) na $XR_BIN"
  else
    log "UWAGA: setcap nieudane -- -t pobiegnie bez SCHED_FIFO (miekka degradacja)"
  fi
fi

# Priorytet FIFO 50 (= domyslny) JAWNIE, po wyniku negatywnym z 2026-07-16:
# proba rt_priority=80 (powyzej watkow IRQ PREEMPT_RT @50) skrocila piki E1
# 43->25ms, ale ZAGLODZILA system -- siec przestala odpowiadac >15s, sesja SSH
# nadzorcy padla, kampania przerwana (JOURNAL.md, kampania 7). Wniosek: piki
# housekeepingu nalezy adresowac izolacja rdzenia (isolcpus / IRQ affinity),
# nie eskalacja priorytetu ponad watki przerwan.
cat > "$WORKDIR/study.toml" <<'EOF'
[scheduling]
rt_priority = 50
EOF

RDB_BENCH_CSV="$WORKDIR/e1_probe.csv" \
  timeout "${RUN_TIMEOUT}s" $PIN_XR xretractor study.rql -k -m "$SAMPLES" -t -g study.toml &
XRETRACTOR_PID=$!
sleep 2  # czas na start serwera IPC przed podpieciem klientow

for i in $(seq 1 "$CLIENTS"); do
  if [ "$SINK" = "nc" ]; then
    $PIN_BG xqry -s "$STREAM_NAME" -r 2>>"$WORKDIR/xqry_${i}.err" | nc -q1 127.0.0.1 "$NC_PORT" &
  else
    $PIN_BG xqry -s "$STREAM_NAME" -r >/dev/null 2>>"$WORKDIR/xqry_${i}.err" &
  fi
  CLIENT_PIDS+=($!)
done

log "xretractor (pid $XRETRACTOR_PID) w toku, $CLIENTS klient(ow) xqry podpietych, limit ${RUN_TIMEOUT}s..."
wait "$XRETRACTOR_PID" || log "xretractor zakonczyl sie kodem $? (sprawdz e1_probe.csv/logi)"

for pid in "${CLIENT_PIDS[@]}"; do kill "$pid" 2>/dev/null || true; done
kill "$METRICS_PID" 2>/dev/null || true
[ -n "$NC_PID" ] && kill "$NC_PID" 2>/dev/null || true
wait 2>/dev/null || true

snapshot_state "PO badaniu" "$WORKDIR/state_after.md"

if [ ! -s "$WORKDIR/e1_probe.csv" ]; then
  log "UWAGA: e1_probe.csv puste/brak -- xretractor prawdopodobnie zbudowany BEZ -DRDB_BENCH_PROBE=ON"
fi

# --- Raport ------------------------------------------------------------------
{
  echo "# Wyniki badania ${STUDY_ID} -- kampania ${CAMPAIGN}"
  echo
  echo "- data: $(date -Is)"
  echo "- czestosc napływu: ${RATE_HZ} Hz (natywna 205: 360 Hz)"
  echo "- liczba klientow xqry: ${CLIENTS} (sink=${SINK})"
  echo "- liczba probek (-m): ${SAMPLES}"
  echo
  echo "## E1 / E2E (sonda RDB_BENCH_CSV)"
  echo '```'
  if [ -s "$WORKDIR/e1_probe.csv" ]; then
    python3 "$REPO_ROOT/examples/ecg/e1_stats.py" "$WORKDIR/e1_probe.csv" --fs "$RATE_HZ" 2>&1
  else
    echo "brak danych z sondy"
  fi
  echo '```'
  echo
  echo "## Metryki systemowe w trakcie badania"
  echo '```'
  awk -F, 'NR>1{load+=$2; mem+=$3; if($5!="NA"){temp+=$5; n++}; c++}
           END{if(c>0) printf "srednie load1=%.2f mem_used_kb=%.0f", load/c, mem/c;
               if(n>0) printf " temp_mC=%.0f", temp/n; print ""}' "$WORKDIR/metrics.csv"
  echo '```'
  echo
  echo "## Pliki w tym katalogu"
  echo "- state_before.md / state_after.md -- pelny stan maszyny przed/po"
  echo "- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)"
  echo "- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania"
} > "$RESULTS_DIR/results.md"

cp "$WORKDIR/state_before.md" "$WORKDIR/state_after.md" "$WORKDIR/e1_probe.csv" "$WORKDIR/metrics.csv" "$RESULTS_DIR/" 2>/dev/null || true

# --- Commit + push (amend na kazde kolejne badanie CALEGO eksperymentu na
# tym branchu, niezaleznie od kampanii -- pkt 5). Marker jest przypisany do
# brancha, nie do kampanii: kolejne kampanie (rate, clients, ...) na tym samym
# branchu daja jeden narastajacy commit, a nie osobny commit per kampania.
cd "$REPO_ROOT"
git add "$RESULTS_DIR"
CAMPAIGN_README="$(dirname "$RESULTS_DIR")/README.md"
[ -f "$CAMPAIGN_README" ] && git add "$CAMPAIGN_README"

MARKER="Experiment-Branch: ${BRANCH}"
COMMIT_MSG="$(printf 'eksperyment %s: wyniki kampanii %s, badanie %s\n\n%s' \
  "$BRANCH" "$CAMPAIGN" "$STUDY_ID" "$MARKER")"

if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  log "Amend istniejacego commita eksperymentu..."
  git commit --amend -m "$COMMIT_MSG"
else
  log "Pierwszy commit tego eksperymentu na branchu $BRANCH..."
  git commit -m "$COMMIT_MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"

rm -rf "$WORKDIR"
log "Badanie $STUDY_ID zakonczone i wyslane."
