#!/bin/bash
# Kampania baseline-numpy (REQUIREMENTS.md, sekcja "Plan badawczy — kampanie
# baseline", priorytet 1; cel: sekcja 7.5(i) artykułu main-debs.tex).
# Uruchamiane NA workerze (bezposrednio lub przez SSH z maszyny nadzorcy).
#
# Mierzy potok Pan-Tompkinsa w float64 (config/pan_tompkins_numpy.py) w dwoch
# trybach raportowanych OSOBNO (nie wolno ich usredniac ani porownywac wprost):
#   slot  — petla probka-po-probce z absolutnymi terminami slotow; sonda CSV
#           zgodna z RDB_BENCH_CSV silnika -> analiza tym samym e1_stats.py;
#           odpowiednik metryki E1/E2E (z narzutem interpretera CPython).
#   batch — wektoryzowany przebieg calosci; przepustowosc batch bez semantyki
#           slotowej.
# Dyscyplina srodowiska identyczna z kampaniami QRS: dane wylacznie w /dev/shm
# (pkt 17/18), governor performance (przywracany trapem), taskset -c 3 dla
# procesu mierzonego, sampler metryk na 0-2, SCHED_FIFO 50 nadawane przez
# `sudo -n chrt -f -p 50 <pid>` w fazie settle procesu Pythona (odpowiednik
# sciezki setcap dla xretractor: proces biegnie jako zwykly uzytkownik,
# wlasnosc plikow bez zmian; brak sudo = miekka degradacja, odnotowana
# w META w results.md).
#
# Usage: run_numpy_baseline.sh <branch> [rate_hz=360] [samples=20000] \
#                              [batch_samples=650000] [batch_repeats=5]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
source "$REPO_ROOT/examples/experiment/lib/common.sh"

BRANCH="${1:?Usage: run_numpy_baseline.sh <branch> [rate_hz] [samples] [batch_samples] [batch_repeats]}"
RATE_HZ="${2:-360}"
SAMPLES="${3:-20000}"
BATCH_SAMPLES="${4:-650000}"
BATCH_REPEATS="${5:-5}"

command -v python3 >/dev/null || die "python3 nie jest na PATH"
python3 -c "import numpy" 2>/dev/null || die "brak numpy w srodowisku python3 (venv workera: pip install numpy)"

ECG_SRC="$REPO_ROOT/examples/ecg/rec205"
[ -f "$ECG_SRC/rec205" ] || die "Brak $ECG_SRC/rec205 -- uruchom examples/ecg/build.sh najpierw"

[ -d /dev/shm ] || die "/dev/shm niedostepny -- brak tmpfs do bezpiecznego badania (pkt 17)"
WORKDIR="/dev/shm/numpy-baseline"
RESULTS_DIR="$REPO_ROOT/examples/experiment/results/baseline-numpy"
PIPELINE="$REPO_ROOT/examples/experiment/config/pan_tompkins_numpy.py"
[ -f "$PIPELINE" ] || die "Brak $PIPELINE"

XR_CPU=3
BG_CPUS="0-2"
METRICS_PID=""
PYPID=""
ORIG_GOVERNOR=""

cleanup() {
  [ -n "$METRICS_PID" ] && kill "$METRICS_PID" 2>/dev/null || true
  [ -n "$PYPID" ] && kill "$PYPID" 2>/dev/null || true
  if [ -n "$ORIG_GOVERNOR" ]; then
    echo "$ORIG_GOVERNOR" | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null 2>&1 || true
    ORIG_GOVERNOR=""
  fi
  wait 2>/dev/null || true
}
trap cleanup EXIT

log "=== Kampania baseline-numpy: rate=${RATE_HZ}Hz samples=$SAMPLES batch=${BATCH_SAMPLES}x${BATCH_REPEATS} ==="

cd "$REPO_ROOT"
git fetch origin "$BRANCH" --quiet
git checkout "$BRANCH" >/dev/null 2>&1
git reset --hard "origin/$BRANCH" >/dev/null

rm -rf "$WORKDIR" && mkdir -p "$WORKDIR"
mkdir -p "$RESULTS_DIR"
cp "$ECG_SRC/rec205" "$ECG_SRC/bp_coef.txt" "$ECG_SRC/d_coef.txt" "$WORKDIR/"

snapshot_state() {
  local label="$1" out="$2"
  {
    echo "# Stan maszyny -- $label"
    echo
    echo "- data: $(date -Is)"
    echo "- badanie: campaign=baseline-numpy rate_hz=$RATE_HZ samples=$SAMPLES batch=${BATCH_SAMPLES}x${BATCH_REPEATS}"
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

if sudo -n true 2>/dev/null; then
  ORIG_GOVERNOR=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)
  echo performance | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null
  log "Governor CPU: performance na czas badania (bylo: $ORIG_GOVERNOR)"
else
  log "UWAGA: brak bezhaslowego sudo -- governor bez zmian, FIFO niedostepne (miekka degradacja)"
fi

snapshot_state "PRZED badaniem" "$WORKDIR/state_before.md"

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
(
  command -v taskset >/dev/null && taskset -p -c "$BG_CPUS" $BASHPID >/dev/null 2>&1
  sample_metrics "$WORKDIR/metrics.csv"
) &
METRICS_PID=$!

PIN_XR=""
command -v taskset >/dev/null && PIN_XR="taskset -c $XR_CPU"

# --- Tryb slot (odpowiednik E1/E2E) ------------------------------------------
cd "$WORKDIR"
RUN_TIMEOUT=$(( SAMPLES * 30 / 1000 + 120 ))
log "Tryb slot: ${RATE_HZ} Hz, ${SAMPLES} probek (limit ${RUN_TIMEOUT}s)..."
timeout "${RUN_TIMEOUT}s" $PIN_XR python3 "$PIPELINE" \
  --rec rec205 --bp bp_coef.txt --d d_coef.txt \
  --mode slot --rate "$RATE_HZ" --samples "$SAMPLES" \
  --probe "$WORKDIR/probe_slot.csv" --settle 3 \
  > "$WORKDIR/slot_stdout.log" 2>&1 &
PYPID=$!

# Nadanie SCHED_FIFO 50 z zewnatrz w fazie settle (wlasnosc plikow bez zmian).
# $PYPID to pid timeout/taskset, nie interpretera — wlasciwy pid Python drukuje
# jako "PID <n>" przed settle; chrt na zly pid nie dziedziczy sie na dziecko.
PY_REAL_PID=""
for _ in $(seq 1 20); do
  PY_REAL_PID=$(awk '/^PID /{print $2; exit}' "$WORKDIR/slot_stdout.log" 2>/dev/null)
  [ -n "$PY_REAL_PID" ] && break
  sleep 0.2
done
if [ -n "$PY_REAL_PID" ] && sudo -n chrt -f -p 50 "$PY_REAL_PID" 2>/dev/null; then
  log "SCHED_FIFO 50 nadane procesowi slot (pid $PY_REAL_PID)"
else
  log "UWAGA: chrt nieudane -- tryb slot pobiegnie na SCHED_OTHER (odnotowane w META)"
fi

wait "$PYPID" || log "tryb slot zakonczyl sie kodem $? (sprawdz slot_stdout.log)"
PYPID=""
[ -s "$WORKDIR/probe_slot.csv" ] || die "Brak sondy trybu slot (patrz slot_stdout.log)"

# --- Tryb batch ---------------------------------------------------------------
log "Tryb batch: ${BATCH_SAMPLES} probek x ${BATCH_REPEATS} powtorzen..."
$PIN_XR python3 "$PIPELINE" \
  --rec rec205 --bp bp_coef.txt --d d_coef.txt \
  --mode batch --samples "$BATCH_SAMPLES" --repeats "$BATCH_REPEATS" \
  > "$WORKDIR/batch_stdout.log" 2>&1 \
  || log "tryb batch zakonczyl sie kodem $? (sprawdz batch_stdout.log)"

kill "$METRICS_PID" 2>/dev/null || true; METRICS_PID=""
wait 2>/dev/null || true

snapshot_state "PO badaniu" "$WORKDIR/state_after.md"

# --- Raport --------------------------------------------------------------------
{
  echo "# Wyniki kampanii baseline-numpy"
  echo
  echo "- data: $(date -Is)"
  echo "- cel: baseline float64 dla 7.5(i) main-debs.tex (koszt dokladnej semantyki wymiernej)"
  echo "- potok: etapy identyczne z rec205-detect.rql, float64, prawdziwe dzielenie"
  echo "- implementacja: config/pan_tompkins_numpy.py (orientacja okna jak ref_float.py)"
  echo "- srodowisko: $(uname -r), governor performance, taskset -c ${XR_CPU}, chrt -f 50 (slot)"
  echo "- cmdline: $(cat /proc/cmdline)"
  echo "- tryby NIEPOROWNYWALNE wprost: slot = koszt na slot (z narzutem CPython),"
  echo "  batch = przepustowosc wektoryzowana bez semantyki slotowej"
  echo
  echo "## Wersje (przypiete dla odtwarzalnosci)"
  echo '```'
  python3 -c "import sys, numpy; print(f'python {sys.version.split()[0]}'); print(f'numpy {numpy.__version__}')"
  echo '```'
  echo
  echo "## Tryb slot: ${RATE_HZ} Hz, ${SAMPLES} probek (sonda zgodna z RDB_BENCH_CSV)"
  echo '```'
  grep -E "^META " "$WORKDIR/slot_stdout.log" || true
  python3 "$REPO_ROOT/examples/ecg/e1_stats.py" "$WORKDIR/probe_slot.csv" --fs "$RATE_HZ" 2>&1
  echo '```'
  echo
  echo "## Tryb batch: ${BATCH_SAMPLES} probek x ${BATCH_REPEATS}"
  echo '```'
  cat "$WORKDIR/batch_stdout.log"
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
  echo "- probe_slot.csv -- surowa sonda trybu slot (iter,compute_ns,wake_lag_ns,e2e_ns)"
  echo "- slot_stdout.log / batch_stdout.log -- pelne wyjscie obu trybow (w tym META)"
  echo "- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania"
} > "$RESULTS_DIR/results.md"

cp "$WORKDIR/state_before.md" "$WORKDIR/state_after.md" "$WORKDIR/probe_slot.csv" \
   "$WORKDIR/slot_stdout.log" "$WORKDIR/batch_stdout.log" "$WORKDIR/metrics.csv" \
   "$RESULTS_DIR/" 2>/dev/null || true

# --- Commit + push zgodnie z konwencja eksperymentu (amend na wspolnym commicie)
cd "$REPO_ROOT"
git add "$RESULTS_DIR"
MARKER="Experiment-Branch: ${BRANCH}"
MSG="eksperyment ${BRANCH}: kampania baseline-numpy

${MARKER}"
if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  git commit --amend -m "$MSG"
else
  git commit -m "$MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"
rm -rf "$WORKDIR"
log "Kampania baseline-numpy zakonczona i wyslana."
