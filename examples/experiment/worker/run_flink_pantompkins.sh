#!/bin/bash
# Kampania baseline-flink, krok 2 (REQUIREMENTS.md, "Plan badawczy --
# kampanie baseline", priorytet 2; cel: 7.5(ii) main-debs.tex). Uruchamiane
# NA workerze. Wymaga wykonanego kroku 1 (run_flink_baseline.sh) --
# zainstalowanego OpenJDK 17 + Apache Flink pod ~/flink-install/flink-<wersja>.
#
# Kompiluje config/PanTompkinsFlinkJob.java (rownowazny dataflow
# Pan-Tompkinsa, patrz komentarz w tym pliku), pakuje do jara i uruchamia
# pelna kampanie (domyslnie 20000 próbek @360 Hz) w trybie local/MiniCluster
# (jeden JVM), z ta sama dyscyplina srodowiska co kampanie QRS/numpy:
# governor performance, taskset -c 3 (rdzen izolowany), dane w /dev/shm.
#
# Odstepstwo udokumentowane: SCHED_FIFO NIE jest nadawany JVM-owi (w
# przeciwienstwie do numpy/silnika) -- JVM ma wiele wlasnych wątkow (GC, JIT
# compiler threads) i podniesienie calego procesu do FIFO powtarzaloby ryzyko
# z Kampanii 7 (FIFO 80 > IRQ = zagłodzenie systemu), tym razem z
# nieprzewidywalna liczba wątkow. Governor+taskset to kontrola srodowiska
# uznana za wystarczajaca dla kroku eksploracyjnego.
#
# Usage: run_flink_pantompkins.sh <branch> [flink_version=2.3.0] [rate_hz=360] \
#                                  [samples=20000] [jm_mem=768m] [tm_mem=1024m]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
source "$REPO_ROOT/examples/experiment/lib/common.sh"

BRANCH="${1:?Usage: run_flink_pantompkins.sh <branch> [flink_version] [rate_hz] [samples] [jm_mem] [tm_mem]}"
FLINK_VERSION="${2:-2.3.0}"
RATE_HZ="${3:-360}"
SAMPLES="${4:-20000}"
JM_MEM="${5:-768m}"
TM_MEM="${6:-1024m}"

FLINK_HOME="$HOME/flink-install/flink-${FLINK_VERSION}"
JAVA_HOME_CANDIDATE="/usr/lib/jvm/java-17-openjdk-arm64"
export JAVA_HOME="$JAVA_HOME_CANDIDATE"
export PATH="$JAVA_HOME/bin:$PATH"

[ -d "$FLINK_HOME" ] || die "Brak $FLINK_HOME -- wykonaj najpierw run_flink_baseline.sh (krok 1)"

ECG_SRC="$REPO_ROOT/examples/ecg/rec205"
[ -f "$ECG_SRC/rec205" ] || die "Brak $ECG_SRC/rec205 -- uruchom examples/ecg/build.sh najpierw"

[ -d /dev/shm ] || die "/dev/shm niedostepny -- brak tmpfs do bezpiecznego badania (pkt 17)"
WORKDIR="/dev/shm/flink-pantompkins"
RESULTS_DIR="$REPO_ROOT/examples/experiment/results/baseline-flink"
JOB_SRC="$REPO_ROOT/examples/experiment/config/PanTompkinsFlinkJob.java"
[ -f "$JOB_SRC" ] || die "Brak $JOB_SRC"

XR_CPU=3
BG_CPUS="0-2"
METRICS_PID=""
ORIG_GOVERNOR=""

cleanup() {
  [ -n "$METRICS_PID" ] && kill "$METRICS_PID" 2>/dev/null || true
  if [ -n "$ORIG_GOVERNOR" ]; then
    echo "$ORIG_GOVERNOR" | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null 2>&1 || true
    ORIG_GOVERNOR=""
  fi
  wait 2>/dev/null || true
}
trap cleanup EXIT

log "=== Kampania baseline-flink krok 2: Flink ${FLINK_VERSION}, rate=${RATE_HZ}Hz samples=$SAMPLES ==="

cd "$REPO_ROOT"
git fetch origin "$BRANCH" --quiet
git checkout "$BRANCH" >/dev/null 2>&1
git reset --hard "origin/$BRANCH" >/dev/null

rm -rf "$WORKDIR" && mkdir -p "$WORKDIR/classes"
mkdir -p "$RESULTS_DIR"
cp "$ECG_SRC/rec205" "$ECG_SRC/bp_coef.txt" "$ECG_SRC/d_coef.txt" "$WORKDIR/"

log "Kompiluje $JOB_SRC ..."
javac -cp "$FLINK_HOME/lib/flink-dist-${FLINK_VERSION}.jar" -d "$WORKDIR/classes" "$JOB_SRC"
mkdir -p "$WORKDIR/META-INF"
printf 'Manifest-Version: 1.0\nMain-Class: PanTompkinsFlinkJob\n' > "$WORKDIR/META-INF/MANIFEST.MF"
jar cfm "$WORKDIR/pan-tompkins-flink.jar" "$WORKDIR/META-INF/MANIFEST.MF" -C "$WORKDIR/classes" .

snapshot_state() {
  local label="$1" out="$2"
  {
    echo "# Stan maszyny -- $label"
    echo
    echo "- data: $(date -Is)"
    echo "- badanie: campaign=baseline-flink krok=2 (pantompkins) rate_hz=$RATE_HZ samples=$SAMPLES"
    echo
    echo '## uname'; echo '```'; uname -a; echo '```'
    echo '## Pamiec'; echo '```'; free -h; echo '```'
    echo '## Load average'; echo '```'; cat /proc/loadavg; echo '```'
    echo '## Temperatura'
    echo '```'
    for z in /sys/class/thermal/thermal_zone*/temp; do
      [ -r "$z" ] || continue
      printf '%s: %d m°C\n' "$z" "$(cat "$z")"
    done
    echo '```'
    echo '## Governor i czestotliwosci CPU'
    echo '```'
    for c in /sys/devices/system/cpu/cpu[0-9]*/cpufreq; do
      [ -r "$c/scaling_governor" ] || continue
      printf '%s: governor=%s cur=%s kHz\n' "$(basename "$(dirname "$c")")" \
        "$(cat "$c/scaling_governor")" "$(cat "$c/scaling_cur_freq")"
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
  log "UWAGA: brak bezhaslowego sudo -- governor bez zmian (miekka degradacja)"
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

PIN=""
command -v taskset >/dev/null && PIN="taskset -c $XR_CPU"

cd "$FLINK_HOME"
RUN_TIMEOUT=$(( SAMPLES * 30 / 1000 + 180 ))
log "Pelny przebieg: ${RATE_HZ} Hz, ${SAMPLES} probek (limit ${RUN_TIMEOUT}s)..."
if timeout "${RUN_TIMEOUT}s" $PIN bin/flink run -t local \
    -Djobmanager.memory.process.size="$JM_MEM" \
    -Dtaskmanager.memory.process.size="$TM_MEM" \
    "$WORKDIR/pan-tompkins-flink.jar" \
    --rec "$WORKDIR/rec205" --bp "$WORKDIR/bp_coef.txt" --d "$WORKDIR/d_coef.txt" \
    --rate "$RATE_HZ" --samples "$SAMPLES" \
    --probe "$WORKDIR/probe_flink.csv" --sink "$WORKDIR/sink_flink.txt" \
    > "$WORKDIR/run_stdout.log" 2>&1; then
  RUN_EXIT=0
else
  RUN_EXIT=$?
  log "UWAGA: przebieg zakonczyl sie kodem ${RUN_EXIT} (patrz run_stdout.log)"
fi

kill "$METRICS_PID" 2>/dev/null || true; METRICS_PID=""
wait 2>/dev/null || true

snapshot_state "PO badaniu" "$WORKDIR/state_after.md"

[ -s "$WORKDIR/probe_flink.csv" ] || die "Brak sondy (probe_flink.csv) -- patrz run_stdout.log"

{
  echo "# Wyniki kampanii baseline-flink -- krok 2: rownowazny dataflow Pan-Tompkinsa"
  echo
  echo "- data: $(date -Is)"
  echo "- cel: 7.5(ii) main-debs.tex -- porownanie modelu wykonania DSMS glownego"
  echo "  nurtu (Flink DataStream API) z silnikiem RetractorDB i baseline-numpy"
  echo "- Flink: ${FLINK_VERSION}, tryb local/MiniCluster (jeden JVM embedded w"
  echo "  \`bin/flink run -t local\`), parallelism=1, checkpointing wylaczony"
  echo "- limity pamieci: jobmanager.memory.process.size=${JM_MEM},"
  echo "  taskmanager.memory.process.size=${TM_MEM}"
  echo "- srodowisko: $(uname -r), governor performance, taskset -c ${XR_CPU}"
  echo "- **ODSTEPSTWO**: SCHED_FIFO NIE nadany JVM-owi (wielowatkowy GC/JIT --"
  echo "  ryzyko jak w Kampanii 7, patrz komentarz w skrypcie); tylko"
  echo "  governor+taskset, tak samo jak reszta srodowiska"
  echo "- exit_code przebiegu: ${RUN_EXIT}"
  echo
  echo "## Wersje"
  echo '```'
  java -version 2>&1
  echo "Flink ${FLINK_VERSION}"
  echo '```'
  echo
  echo "## Log przebiegu (naglowek META + koniec)"
  echo '```'
  grep -E "^META " "$WORKDIR/run_stdout.log" || true
  tail -10 "$WORKDIR/run_stdout.log"
  echo '```'
  echo
  echo "## Analiza sondy (e1_stats.py, format RDB_BENCH_CSV)"
  echo '```'
  python3 "$REPO_ROOT/examples/ecg/e1_stats.py" "$WORKDIR/probe_flink.csv" --fs "$RATE_HZ" 2>&1
  echo '```'
  echo
  echo "## UWAGA INTERPRETACYJNA (obowiazkowa przed cytowaniem w artykule)"
  echo "compute_ns mierzy od odebrania rekordu przez zrodlo (PO pacingu) do"
  echo "konca ostatniego etapu obliczen (Threshold, PRZED sinkiem). Dzieki"
  echo "chainowaniu operatorow (map->map->map->map->sink, ta sama parallelism,"
  echo "brak keyBy/shuffle) caly potok dziala w JEDNYM wątku -- ale definicja"
  echo "compute_ns wciaz zawiera JEDNO przejscie przez kolejke Source->Chain"
  echo "(Flink nie chainuje zrodel), czego nie ma numpy/silnik (tam jedna"
  echo "synchroniczna funkcja). e2e_ns/wake_lag_ns moga zawierac tail od"
  echo "rozgrzewki JIT (kompilacja C1/C2 w pierwszych iteracjach) -- odrebny"
  echo "mechanizm od pikow housekeepingu jadra opisanych dla silnika (K6/K9)."
  echo
  echo "## Metryki systemowe w trakcie badania"
  echo '```'
  awk -F, 'NR>1{load+=$2; mem+=$3; if($5!="NA"){temp+=$5; n++}; c++}
           END{if(c>0) printf "srednie load1=%.2f mem_used_kb=%.0f", load/c, mem/c;
               if(n>0) printf " temp_mC=%.0f", temp/n; print ""}' "$WORKDIR/metrics.csv"
  echo '```'
  echo
  echo "## Pliki w tym katalogu"
  echo "- state_before.md / state_after.md -- stan maszyny przed/po (krok 2)"
  echo "- probe_flink.csv -- surowa sonda (iter,compute_ns,wake_lag_ns,e2e_ns)"
  echo "- run_stdout.log -- pelne wyjscie \`bin/flink run\` (w tym META)"
  echo "- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania"
  echo "- PanTompkinsFlinkJob.java -- kopia zrodla joba na moment przebiegu"
} > "$RESULTS_DIR/results_pantompkins.md"

cp "$WORKDIR/state_before.md" "$RESULTS_DIR/state_before_pantompkins.md" 2>/dev/null || true
cp "$WORKDIR/state_after.md" "$RESULTS_DIR/state_after_pantompkins.md" 2>/dev/null || true
cp "$WORKDIR/probe_flink.csv" "$WORKDIR/run_stdout.log" "$WORKDIR/metrics.csv" "$RESULTS_DIR/" 2>/dev/null || true
cp "$JOB_SRC" "$RESULTS_DIR/" 2>/dev/null || true

cd "$REPO_ROOT"
git add "$RESULTS_DIR"
MARKER="Experiment-Branch: ${BRANCH}"
MSG="eksperyment ${BRANCH}: kampania baseline-flink krok 2 (dataflow Pan-Tompkinsa)

${MARKER}"
if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  git commit --amend -m "$MSG"
else
  git commit -m "$MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"
rm -rf "$WORKDIR"
log "Kampania baseline-flink krok 2 zakonczona i wyslana (exit przebiegu: ${RUN_EXIT})."
