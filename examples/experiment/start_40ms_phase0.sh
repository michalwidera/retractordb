#!/bin/bash
# Uruchamiane NA maszynie nadzorcy. Faza 0 sledztwa zdarzen ~40 ms
# (JOURNAL.md 2026-07-18, plan sledztwa): dwa badania dyskryminujace
# "platforma system-wide" od "aktywnosc silnika":
#   S1 pacer-solo:    proces-cien SAM na rdzeniu silnika (dlugi przebieg)
#   S2 engine-shadow: silnik na rdzeniu 3 + cien na rdzeniu 2, N powtorzen
# Miedzy badaniami restart workera (REQUIREMENTS.md pkt 4).
#
# Branch eksperymentu: experiment/40ms -- ZGODNIE Z WYJATKIEM w
# REQUIREMENTS.md pkt 23 (2026-07-18) branch ZAWIERA skrypty badawcze
# (potencjalnie slepa uliczka -- dlatego nie na masterze) i worker buduje
# binarki Z BRANCHA EKSPERYMENTU, nie z mastera. Squash do mastera dopiero
# po jednoznacznym ustaleniu przyczyny.
#
# Usage:
#   examples/experiment/start_40ms_phase0.sh [opcje]
# Opcje:
#   --worker HOST         alias SSH workera (domyslnie: worker)
#   --worker-repo PATH    repo na workerze (domyslnie: /home/michal/retractordb)
#   --branch NAME         branch eksperymentu (domyslnie: experiment/40ms)
#   --s1-samples N        sloty pacer-solo (domyslnie 200000, ~9 min @360Hz)
#   --s2-samples N        probki silnika na powtorzenie (domyslnie 20000)
#   --s2-repeats N        powtorzenia engine-shadow (domyslnie 5)
#   --skip-build          nie buduj na workerze (binarki z sonda juz zainstalowane)
#   --reboot-timeout SEK  czekanie na powrot workera (domyslnie 600)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && cd .. && pwd)"
# shellcheck source=lib/common.sh
source "$SCRIPT_DIR/lib/common.sh"

WORKER_HOST="worker"
WORKER_REPO="/home/michal/retractordb"
BRANCH="experiment/40ms"
S1_SAMPLES=200000
S2_SAMPLES=20000
S2_REPEATS=5
SKIP_BUILD=0
REBOOT_TIMEOUT=600

while [ $# -gt 0 ]; do
  case "$1" in
    --worker) WORKER_HOST="$2"; shift 2 ;;
    --worker-repo) WORKER_REPO="$2"; shift 2 ;;
    --branch) BRANCH="$2"; shift 2 ;;
    --s1-samples) S1_SAMPLES="$2"; shift 2 ;;
    --s2-samples) S2_SAMPLES="$2"; shift 2 ;;
    --s2-repeats) S2_REPEATS="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --reboot-timeout) REBOOT_TIMEOUT="$2"; shift 2 ;;
    *) die "Nieznana opcja: $1" ;;
  esac
done

log "=== Faza 0 sledztwa 40ms na branchu $BRANCH (worker=$WORKER_HOST) ==="

cd "$REPO_ROOT"
git show-ref --verify --quiet "refs/heads/$BRANCH" \
  || die "Branch $BRANCH nie istnieje lokalnie -- utworz go i zacommituj skrypty przed startem"
git checkout "$BRANCH"
[ -z "$(git status --short)" ] || die "Working tree nadzorcy nie jest czysty"
git push -u origin "$BRANCH"

log "Przygotowanie workera ($WORKER_HOST:$WORKER_REPO): checkout $BRANCH..."
ssh_worker "$WORKER_HOST" "
  set -e
  cd '$WORKER_REPO'
  [ -z \"\$(git status --short)\" ] || { echo 'BLAD: working tree workera nie jest czyste' >&2; exit 1; }
  git fetch origin '$BRANCH'
  git checkout '$BRANCH'
  git reset --hard 'origin/$BRANCH'
"

if [ "$SKIP_BUILD" -ne 1 ]; then
  log "Budowanie na workerze Z BRANCHA $BRANCH z -DRDB_BENCH_PROBE=ON (moze potrwac)..."
  ssh_worker "$WORKER_HOST" "cd '$WORKER_REPO' && scripts/buildrdb.sh conan ninja probe && cd build/Release && ninja install"
else
  log "Pomijam budowanie (--skip-build)."
fi

log "--- Badanie S1: pacer-solo ($S1_SAMPLES slotow @360Hz, ~$((S1_SAMPLES / 360 / 60)) min) ---"
ssh_worker "$WORKER_HOST" \
  "'$WORKER_REPO/examples/experiment/worker/run_40ms_phase0.sh' '$BRANCH' pacer-solo '$S1_SAMPLES'" \
  || die "Badanie pacer-solo nie powiodlo sie -- przerywam (bez restartu workera)"

log "Restart maszyny workera przed S2 (pkt 4)..."
ssh_worker "$WORKER_HOST" "sudo -n reboot" || true
wait_for_worker "$WORKER_HOST" "$REBOOT_TIMEOUT"

log "--- Badanie S2: engine-shadow ($S2_REPEATS x $S2_SAMPLES probek @360Hz) ---"
ssh_worker "$WORKER_HOST" \
  "'$WORKER_REPO/examples/experiment/worker/run_40ms_phase0.sh' '$BRANCH' engine-shadow '$S2_SAMPLES' '$S2_REPEATS'" \
  || die "Badanie engine-shadow nie powiodlo sie"

log "=== Faza 0 zakonczona. Wyniki: examples/experiment/results/40ms/ na branchu $BRANCH ==="
log "Nastepny krok: analiza w JOURNAL.md i decyzja o Fazie 1 (ablacja) lub 2 (ftrace)."
