#!/bin/bash
# Test kontrastowy dsp-simple-fir (REQUIREMENTS.md, @note koncowy).
# Uruchamiane NA workerze. Cel: okreslic, jak szybki naplyw danych przetwarza
# PROSTY filtr FIR (4 etapy: okno@25 -> mnozenie -> sumc -> zlaczenie), dla
# kontrastu z pelnym potokiem QRS (~14 etapow). Baza: test/IntegrationTest_parallel/dsp,
# zmodyfikowana zgodnie z metodyka eksperymentu:
#  - zero artefaktow: wszystkie SELECT VOLATILE, bez STORAGE, praca w /dev/shm
#    (przepustowosc karty SD wylaczona z pomiaru),
#  - zrodlo: BYTE z /dev/urandom (pamiec, nie SD),
#  - odbior danych jak w badaniach QRS: 1 klient xqry -> /dev/null (rdzenie 0-2),
#  - pelna kontrola srodowiska: governor performance, taskset -c 3, -t (FIFO 50),
#  - eskalacja tempa: kolejne stopnie tylko, gdy E1 (mediana, rdzen obliczen)
#    miesci sie w budzecie biezacego stopnia -- przekroczenie przerywa test.
#
# Usage: run_fir_contrast.sh <branch> [rates="1000 2000 3000"] [samples=20000]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
source "$REPO_ROOT/examples/experiment/lib/common.sh"

BRANCH="${1:?Usage: run_fir_contrast.sh <branch> [rates] [samples]}"
RATES="${2:-1000 2000 3000}"
SAMPLES="${3:-20000}"

command -v xretractor >/dev/null || die "xretractor nie jest na PATH"
command -v xqry >/dev/null       || die "xqry nie jest na PATH"
COEF_SRC="$REPO_ROOT/test/IntegrationTest_parallel/dsp/filterremez.txt"
[ -f "$COEF_SRC" ] || die "Brak $COEF_SRC"

RESULTS_DIR="$REPO_ROOT/examples/experiment/results/fir-contrast"
BASE=/dev/shm/fir-contrast
ORIG_GOVERNOR=""
XPID=""
QPID=""

cleanup() {
  [ -n "$QPID" ] && kill "$QPID" 2>/dev/null || true
  [ -n "$XPID" ] && kill "$XPID" 2>/dev/null || true
  if [ -n "$ORIG_GOVERNOR" ]; then
    echo "$ORIG_GOVERNOR" | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null 2>&1 || true
    ORIG_GOVERNOR=""
  fi
  wait 2>/dev/null || true
}
trap cleanup EXIT

cd "$REPO_ROOT"
git fetch origin "$BRANCH" --quiet
git checkout "$BRANCH" >/dev/null 2>&1
git reset --hard "origin/$BRANCH" >/dev/null

rm -rf "$BASE" && mkdir -p "$BASE"
mkdir -p "$RESULTS_DIR"

XR_BIN="$(command -v xretractor)"
if command -v getcap >/dev/null && ! getcap "$XR_BIN" 2>/dev/null | grep -q "cap_sys_nice"; then
  sudo -n setcap cap_sys_nice,cap_ipc_lock+ep "$XR_BIN" 2>/dev/null || log "UWAGA: setcap nieudane"
fi

if sudo -n true 2>/dev/null; then
  ORIG_GOVERNOR=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)
  echo performance | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null
fi

{
  echo "# Test kontrastowy dsp-simple-fir"
  echo
  echo "- data: $(date -Is)"
  echo "- cel: maks. tempo naplywu dla prostego filtru FIR (kontrast z QRS ~14 etapow)"
  echo "- potok: okno@(1,25) -> mnozenie el-po-el -> .sumc -> zlaczenie; VOLATILE, /dev/shm"
  echo "- srodowisko: $(uname -r), governor performance, taskset -c 3, -t (FIFO 50)"
  echo "- cmdline: $(cat /proc/cmdline)"
  echo "- probki na stopien: $SAMPLES; stopnie [Hz]: $RATES"
  echo "- regula stopu: E1 mediana > budzet stopnia -> przerwij eskalacje"
  echo
} > "$RESULTS_DIR/results.md"

STOPPED=""
for RATE in $RATES; do
  WD="$BASE/step_${RATE}"
  mkdir -p "$WD"
  cp "$COEF_SRC" "$WD/"
  # Zapytanie z wersjonowanego pliku konfiguracyjnego; podmiana tempa jak w QRS
  sed "s#1/1000#1/${RATE}#" "$REPO_ROOT/examples/experiment/config/dsp-simple-fir.rql" > "$WD/fir.rql"
  cat > "$WD/study.toml" <<'EOF'
[scheduling]
rt_priority = 50
EOF

  log "FIR stopien ${RATE} Hz (${SAMPLES} probek)..."
  cd "$WD"
  RUN_TIMEOUT=$(( SAMPLES * 30 / 1000 + 120 ))
  RDB_BENCH_CSV="$WD/probe.csv" \
    timeout "${RUN_TIMEOUT}s" taskset -c 3 xretractor fir.rql -k -m "$SAMPLES" -t -g study.toml >/dev/null 2>&1 &
  XPID=$!
  sleep 2
  taskset -c 0-2 xqry -s outputAll -r >/dev/null 2>&1 &
  QPID=$!
  wait "$XPID" || log "xretractor zakonczyl sie kodem $? na stopniu ${RATE}"
  XPID=""
  kill "$QPID" 2>/dev/null || true; QPID=""
  wait 2>/dev/null || true

  [ -s "$WD/probe.csv" ] || die "Brak danych sondy na stopniu ${RATE} (binarka bez RDB_BENCH_PROBE?)"

  cp "$WD/probe.csv" "$RESULTS_DIR/probe_${RATE}hz.csv"
  {
    echo "## Stopien ${RATE} Hz (budzet $(python3 -c "print(f'{1e6/${RATE}:.1f}')") us)"
    echo '```'
    python3 "$REPO_ROOT/examples/ecg/e1_stats.py" "$WD/probe.csv" --fs "$RATE" 2>&1
    echo '```'
    echo
  } >> "$RESULTS_DIR/results.md"

  E1_MED_US=$(python3 - "$WD/probe.csv" <<'EOF'
import csv, statistics, sys
rows = list(csv.reader(open(sys.argv[1])))[1:]
print(f"{statistics.median(int(r[1]) for r in rows)/1000:.1f}")
EOF
)
  BUDGET_US=$(python3 -c "print(f'{1e6/${RATE}:.1f}')")
  log "FIR ${RATE} Hz: E1 mediana ${E1_MED_US} us, budzet ${BUDGET_US} us"
  if python3 -c "exit(0 if ${E1_MED_US} > ${BUDGET_US} else 1)"; then
    STOPPED="stopien ${RATE} Hz: E1 mediana ${E1_MED_US} us > budzet ${BUDGET_US} us -- eskalacja przerwana (regula stopu)"
    log "$STOPPED"
    echo "**${STOPPED}**" >> "$RESULTS_DIR/results.md"
    break
  fi
done
[ -n "$STOPPED" ] || echo "_Wszystkie stopnie w budzecie E1 -- mozna eskalowac dalej._" >> "$RESULTS_DIR/results.md"

# Commit wynikow zgodnie z konwencja eksperymentu (amend na wspolnym commicie)
cd "$REPO_ROOT"
git add "$RESULTS_DIR"
MARKER="Experiment-Branch: ${BRANCH}"
MSG="eksperyment ${BRANCH}: test kontrastowy dsp-simple-fir

${MARKER}"
if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  git commit --amend -m "$MSG"
else
  git commit -m "$MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"
rm -rf "$BASE"
log "Test FIR zakonczony i wyslany."
