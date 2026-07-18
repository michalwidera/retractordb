#!/bin/bash
# Kampania baseline-flink (REQUIREMENTS.md, sekcja "Plan badawczy — kampanie
# baseline", priorytet 2, "expected fail"; cel: sekcja 7.5(ii) main-debs.tex).
# Uruchamiane NA workerze (bezposrednio lub przez SSH z maszyny nadzorcy).
#
# Krok 1 (ten skrypt): instalacja OpenJDK + Apache Flink i smoke test w trybie
# local/MiniCluster (jeden JVM, embedded w procesie `flink run`), heap
# ograniczony (jobmanager+taskmanager <= ~1,75 GB, checkpointing wylaczony
# domyslnie -- nieustawiony execution.checkpointing.interval). Zgodnie z
# zastrzezeniami planu (RAM 4 GB, jitter JVM) najpierw sprawdzamy, czy
# instalacja i trywialny job w ogole dzialaja w budzecie pamieci, przed
# implementacja rownowaznego dataflow Pan-Tompkinsa (krok 2, poza zakresem
# tego skryptu).
#
# Usage: run_flink_baseline.sh <branch> [flink_version=2.3.0] [jm_mem=768m] [tm_mem=1024m]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
source "$REPO_ROOT/examples/experiment/lib/common.sh"

BRANCH="${1:?Usage: run_flink_baseline.sh <branch> [flink_version] [jm_mem] [tm_mem]}"
FLINK_VERSION="${2:-2.3.0}"
JM_MEM="${3:-768m}"
TM_MEM="${4:-1024m}"
SCALA_SUFFIX="scala_2.12"
FLINK_TGZ="flink-${FLINK_VERSION}-bin-${SCALA_SUFFIX}.tgz"
FLINK_URL="https://dlcdn.apache.org/flink/flink-${FLINK_VERSION}/${FLINK_TGZ}"

INSTALL_DIR="$HOME/flink-install"
FLINK_HOME="$INSTALL_DIR/flink-${FLINK_VERSION}"
RESULTS_DIR="$REPO_ROOT/examples/experiment/results/baseline-flink"
JAVA_HOME_CANDIDATE="/usr/lib/jvm/java-17-openjdk-arm64"

log "=== Kampania baseline-flink: instalacja + smoke test (Flink ${FLINK_VERSION}, JM=${JM_MEM} TM=${TM_MEM}) ==="

cd "$REPO_ROOT"
git fetch origin "$BRANCH" --quiet
git checkout "$BRANCH" >/dev/null 2>&1
git reset --hard "origin/$BRANCH" >/dev/null

mkdir -p "$INSTALL_DIR" "$RESULTS_DIR"

snapshot_state() {
  local label="$1" out="$2"
  {
    echo "# Stan maszyny -- $label"
    echo
    echo "- data: $(date -Is)"
    echo "- badanie: campaign=baseline-flink flink=${FLINK_VERSION} jm_mem=${JM_MEM} tm_mem=${TM_MEM}"
    echo
    echo '## uname'
    echo '```'; uname -a; echo '```'
    echo '## dystrybucja'
    echo '```'; cat /etc/os-release 2>/dev/null || echo "brak /etc/os-release"; echo '```'
    echo '## CPU'
    echo '```'; lscpu 2>/dev/null || cat /proc/cpuinfo; echo '```'
    echo '## Pamiec'
    echo '```'; free -h; echo '```'
    echo '## Load average'
    echo '```'; cat /proc/loadavg; echo '```'
    echo '## Temperatura (strefy termiczne)'
    echo '```'
    for z in /sys/class/thermal/thermal_zone*/temp; do
      [ -r "$z" ] || continue
      printf '%s: %d m°C\n' "$z" "$(cat "$z")"
    done
    echo '```'
  } > "$out"
}

snapshot_state "PRZED badaniem" "$RESULTS_DIR/state_before.md"

# --- Krok A: OpenJDK 17 -------------------------------------------------------
if ! command -v java >/dev/null || ! java -version 2>&1 | grep -q '"17\.'; then
  log "Instaluje openjdk-17-jdk-headless..."
  sudo -n apt-get install -y openjdk-17-jdk-headless
else
  log "OpenJDK 17 juz obecny: $(java -version 2>&1 | head -1)"
fi
[ -x "$JAVA_HOME_CANDIDATE/bin/java" ] || die "Brak $JAVA_HOME_CANDIDATE (sprawdz sciezke JVM po instalacji)"
export JAVA_HOME="$JAVA_HOME_CANDIDATE"

# --- Krok B: pobranie + weryfikacja + rozpakowanie Flinka (idempotentne) -----
cd "$INSTALL_DIR"
if [ ! -f "$FLINK_TGZ" ]; then
  log "Pobieram $FLINK_URL ..."
  curl -sS -C - --max-time 900 -o "$FLINK_TGZ" "$FLINK_URL"
fi
if [ ! -f "${FLINK_TGZ}.sha512" ]; then
  curl -sS --max-time 20 -o "${FLINK_TGZ}.sha512" "${FLINK_URL}.sha512"
fi
EXPECTED=$(awk '{print $1}' "${FLINK_TGZ}.sha512")
ACTUAL=$(sha512sum "$FLINK_TGZ" | awk '{print $1}')
[ "$EXPECTED" = "$ACTUAL" ] || die "sha512 niezgodny dla $FLINK_TGZ (pobieranie uszkodzone?)"
log "sha512 zweryfikowany: $FLINK_TGZ"

[ -d "$FLINK_HOME" ] || tar xzf "$FLINK_TGZ"
[ -d "$FLINK_HOME" ] || die "Rozpakowanie nie utworzylo $FLINK_HOME"

# --- Krok C: smoke test w local/MiniCluster (jeden JVM), heap ograniczony ---
cd "$FLINK_HOME"
JAR="examples/streaming/WordCount.jar"
[ -f "$JAR" ] || die "Brak $JAR w dystrybucji Flinka"

RAM_SAMPLE="$RESULTS_DIR/ram_sample.csv"
echo "ts_epoch,mem_used_mb" > "$RAM_SAMPLE"
(
  while true; do
    printf '%s,%s\n' "$(date +%s)" "$(free -m | awk '/Mem:/{print $3}')" >> "$RAM_SAMPLE"
    sleep 0.5
  done
) &
SAMPLER_PID=$!
cleanup() { kill "$SAMPLER_PID" 2>/dev/null || true; }
trap cleanup EXIT

log "Smoke test: bin/flink run -t local (WordCount, JM=${JM_MEM} TM=${TM_MEM})..."
START_NS=$(date +%s%N)
SMOKE_LOG="$RESULTS_DIR/smoke_stdout.log"
if timeout 120 bin/flink run -t local \
    -Djobmanager.memory.process.size="$JM_MEM" \
    -Dtaskmanager.memory.process.size="$TM_MEM" \
    "$JAR" > "$SMOKE_LOG" 2>&1; then
  SMOKE_EXIT=0
else
  SMOKE_EXIT=$?
fi
END_NS=$(date +%s%N)
DURATION_MS=$(( (END_NS - START_NS) / 1000000 ))

kill "$SAMPLER_PID" 2>/dev/null || true
trap - EXIT
wait "$SAMPLER_PID" 2>/dev/null || true

snapshot_state "PO badaniu" "$RESULTS_DIR/state_after.md"

PEAK_RAM_MB=$(awk -F, 'NR>1{print $2}' "$RAM_SAMPLE" | sort -n | tail -1)
JOB_RUNTIME=$(grep -oE "Job Runtime: [0-9]+ ms" "$SMOKE_LOG" || echo "nieznany (patrz smoke_stdout.log)")

{
  echo "# Wyniki kampanii baseline-flink -- krok 1: instalacja + smoke test"
  echo
  echo "- data: $(date -Is)"
  echo "- cel: sprawdzic wykonalnosc instalacji Apache Flink na Pi 400 przed"
  echo "  implementacja rownowaznego dataflow Pan-Tompkinsa (7.5(ii) main-debs.tex)"
  echo "- Flink: ${FLINK_VERSION} (${SCALA_SUFFIX}), tryb: local/MiniCluster (jeden JVM,"
  echo "  embedded w procesie \`bin/flink run -t local\`), checkpointing wylaczony (domyslnie)"
  echo "- limity pamieci: jobmanager.memory.process.size=${JM_MEM},"
  echo "  taskmanager.memory.process.size=${TM_MEM} (razem <= ~1,75 GB, RAM total 3,7 GiB, brak swap)"
  echo "- job smoke test: examples/streaming/WordCount.jar (wbudowany przyklad Flinka)"
  echo
  echo "## Wersje"
  echo '```'
  java -version 2>&1
  echo "Flink ${FLINK_VERSION} (${SCALA_SUFFIX})"
  echo '```'
  echo
  echo "## Wynik smoke testu"
  echo '```'
  echo "exit_code=${SMOKE_EXIT}"
  echo "wall_time_ms=${DURATION_MS}"
  echo "job_runtime=${JOB_RUNTIME}"
  echo "peak_mem_used_mb (system-wide, /proc via free)=${PEAK_RAM_MB}"
  echo '```'
  echo
  echo "## Log smoke testu (ogon)"
  echo '```'
  tail -25 "$SMOKE_LOG"
  echo '```'
  echo
  echo "## Pliki w tym katalogu"
  echo "- state_before.md / state_after.md -- pelny stan maszyny przed/po"
  echo "- smoke_stdout.log -- pelne wyjscie \`bin/flink run\`"
  echo "- ram_sample.csv -- probkowanie RAM co 0,5s w trakcie joba (mem_used, /proc/meminfo)"
  echo
  echo "## Nastepny krok (poza zakresem tego przebiegu)"
  echo "Jesli smoke test przeszedl w budzecie pamieci: implementacja rownowaznego"
  echo "dataflow Pan-Tompkinsa (DataStream API) + pomiar per-rekord, wg zastrzezen"
  echo "REQUIREMENTS.md (jitter GC/JIT, brak semantyki slotowej, izolacja rdzenia"
  echo "nietypowa dla Flinka). Kryterium stopu calej kampanii: budzet 1 dnia roboczego."
} > "$RESULTS_DIR/results.md"

# --- Commit + push zgodnie z konwencja eksperymentu (amend na wspolnym commicie)
cd "$REPO_ROOT"
git add "$RESULTS_DIR"
MARKER="Experiment-Branch: ${BRANCH}"
MSG="eksperyment ${BRANCH}: kampania baseline-flink krok 1 (instalacja + smoke test)

${MARKER}"
if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  git commit --amend -m "$MSG"
else
  git commit -m "$MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"
log "Kampania baseline-flink krok 1 zakonczona i wyslana (exit smoke test: ${SMOKE_EXIT})."
