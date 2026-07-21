#!/bin/bash
# Uruchamiane NA maszynie nadzorcy (/home/michal/github/retractordb, patrz REQUIREMENTS.md @note).
# Steruje cala kampania badawcza: tworzy/aktualizuje branch eksperymentu, przygotowuje
# workera (checkout + build z sonda), a nastepnie dla kazdego badania z pliku CSV:
#   1) zleca workerowi run_study.sh (pkt 9, 10)
#   2) restartuje fizycznie maszyne workera (pkt 4) i czeka az wroci
#
# Wymaga na workerze: bezhaslowego 'sudo reboot' (wpis w /etc/sudoers.d/), np.:
#   michal ALL=(root) NOPASSWD: /usr/sbin/reboot
#
# Usage:
#   examples/experiment/start_supervisor.sh rate       [--branch NAME] [--sink null|nc]
#   examples/experiment/start_supervisor.sh rate_dense [--branch NAME] [--sink null|nc]
#   examples/experiment/start_supervisor.sh clients --rate-hz N [--branch NAME] [--sink null|nc]
#
# 'rate_dense' to ta sama kampania co 'rate' (ten sam uklad kolumn CSV, ten sam cel),
# ale z gestsza drabinka temp -- wlasny plik konfiguracji i wlasny katalog wynikow,
# zeby siatka rzadka i gesta nie mieszaly sie w jednym zestawie.
#
# Opcje:
#   --worker HOST        alias/host SSH workera (domyslnie: worker)
#   --worker-repo PATH    sciezka repo na workerze (domyslnie: /home/michal/retractordb)
#   --skip-build          nie buduj ponownie xretractor/xqry na workerze (zaklada, ze juz sa
#                          zbudowane z -DRDB_BENCH_PROBE=ON i zainstalowane)
#   --reboot-timeout SEK   maks. czas oczekiwania na powrot workera po restarcie (domyslnie 600)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && cd .. && pwd)"
# shellcheck source=lib/common.sh
source "$SCRIPT_DIR/lib/common.sh"

[ $# -ge 1 ] || die "Usage: $0 <rate|rate_dense|clients> [opcje]"
CAMPAIGN="$1"; shift
# CAMPAIGN_KIND rozstrzyga uklad kolumn CSV i tresc celu kampanii; CAMPAIGN zostaje
# nazwa wlasna (katalog wynikow, plik konfiguracji).
case "$CAMPAIGN" in
  rate|rate_dense) CAMPAIGN_KIND="rate" ;;
  clients)         CAMPAIGN_KIND="clients" ;;
  *) die "campaign musi byc 'rate', 'rate_dense' albo 'clients'" ;;
esac

WORKER_HOST="worker"
WORKER_REPO="/home/michal/retractordb"
# Jeden wspolny branch dla calego eksperymentu (wszystkich kampanii) -- nazwa
# NIE zawiera nazwy kampanii, zeby "rate" i "clients" trafialy na ten sam branch.
BRANCH="experiment/$(date +%Y%m%d)"
SINK="null"
RATE_HZ="360"
SKIP_BUILD=0
REBOOT_TIMEOUT=600

while [ $# -gt 0 ]; do
  case "$1" in
    --branch) BRANCH="$2"; shift 2 ;;
    --worker) WORKER_HOST="$2"; shift 2 ;;
    --worker-repo) WORKER_REPO="$2"; shift 2 ;;
    --rate-hz) RATE_HZ="$2"; shift 2 ;;
    --sink) SINK="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --reboot-timeout) REBOOT_TIMEOUT="$2"; shift 2 ;;
    *) die "Nieznana opcja: $1" ;;
  esac
done
[ "$CAMPAIGN_KIND" = "clients" ] && [ "$RATE_HZ" = "360" ] && \
  log "UWAGA: kampania 'clients' uzywa rate-hz=360 (domyslnie). Jesli kampania 'rate' wskazala inna stabilna czestosc, podaj ja przez --rate-hz."

CONFIG_CSV="$SCRIPT_DIR/config/campaign_${CAMPAIGN}.csv"
[ -f "$CONFIG_CSV" ] || die "Brak pliku konfiguracji: $CONFIG_CSV"

log "=== Kampania '$CAMPAIGN' na branchu $BRANCH (worker=$WORKER_HOST, sink=$SINK) ==="

# --- Synchronizacja z origin/master (pkt 26: przed startem branch == master) --
cd "$REPO_ROOT"
log "Sprawdzam synchronizacje master <-> origin/master (nadzorca)..."
git fetch origin master --quiet
git checkout master
[ -z "$(git status --short)" ] || die "Working tree na nadzorcy nie jest czysty -- zacommituj lub odloz zmiany"
[ "$(git rev-parse master)" = "$(git rev-parse origin/master)" ] || \
  die "master na nadzorcy rozjechal sie z origin/master -- zrob 'git pull' (lub push) przed eksperymentem"

log "Sprawdzam synchronizacje master <-> origin/master (worker)..."
ssh_worker "$WORKER_HOST" "
  set -e
  cd '$WORKER_REPO'
  git fetch origin master --quiet
  git checkout master
  [ -z \"\$(git status --short)\" ] || { echo 'BLAD: working tree workera nie jest czyste' >&2; exit 1; }
  [ \"\$(git rev-parse master)\" = \"\$(git rev-parse origin/master)\" ] || \
    { echo 'BLAD: master workera rozjechal sie z origin/master' >&2; exit 1; }
" || die "Worker nie jest zsynchronizowany z origin/master -- przerywam (zaden branch nie zostal utworzony)"

# --- Branch eksperymentu: pkt 6 -- zadnych zmian funkcjonalnych, tylko wyniki --
if git show-ref --verify --quiet "refs/heads/$BRANCH"; then
  git checkout "$BRANCH"
else
  git checkout -b "$BRANCH" origin/master
fi
git push -u origin "$BRANCH"

# --- Przygotowanie workera: ten sam branch, build z sonda (raz na kampanie) --
log "Przygotowanie workera ($WORKER_HOST:$WORKER_REPO)..."
ssh_worker "$WORKER_HOST" "cd '$WORKER_REPO' && git fetch origin '$BRANCH' && git checkout '$BRANCH' && git reset --hard 'origin/$BRANCH'"

if [ "$SKIP_BUILD" -ne 1 ]; then
  log "Budowanie xretractor/xqry na workerze z -DRDB_BENCH_PROBE=ON (moze potrwac)..."
  ssh_worker "$WORKER_HOST" "cd '$WORKER_REPO' && scripts/buildrdb.sh conan ninja probe && cd build/Release && ninja install"
else
  log "Pomijam budowanie (--skip-build)."
fi

CAMPAIGN_RESULTS_DIR="examples/experiment/results/${CAMPAIGN}"

# --- Rotacja poprzednich wynikow (REQUIREMENTS.md, pkt "w przypadku przerwania
# eksperymentu i dostarczenia nowego kodu..."): jesli katalog kampanii ma juz
# wyniki badan (przerwany/poprzedni przebieg), przenosimy je do
# results/<kampania>/rotated/NN/ PRZED wygenerowaniem nowego README i startem
# badania 1 -- zachowuje kontekst zamiast nadpisywac/mieszac stare i nowe dane.
# Rotacja jest od razu commitowana (amend jesli branch juz ma commit tego
# eksperymentu) i wypychana, bo run_study.sh dla badania 1 zacznie od
# 'git reset --hard origin/BRANCH', ktory wymazalby niescommitowana rotacje.
log "Sprawdzam czy sa poprzednie wyniki kampanii '$CAMPAIGN' do rotacji..."
ssh_worker "$WORKER_HOST" "
  set -e
  cd '$WORKER_REPO'
  if [ -d '$CAMPAIGN_RESULTS_DIR' ] && ls '$CAMPAIGN_RESULTS_DIR' 2>/dev/null | grep -q '^study_'; then
    n=1
    while [ -d \"$CAMPAIGN_RESULTS_DIR/rotated/\$(printf '%02d' \$n)\" ]; do n=\$((n+1)); done
    dest=\"$CAMPAIGN_RESULTS_DIR/rotated/\$(printf '%02d' \$n)\"
    mkdir -p \"\$dest\"
    mv '$CAMPAIGN_RESULTS_DIR'/study_* \"\$dest/\"
    [ -f '$CAMPAIGN_RESULTS_DIR/README.md' ] && mv '$CAMPAIGN_RESULTS_DIR/README.md' \"\$dest/\"
    git add -A '$CAMPAIGN_RESULTS_DIR'
    MARKER='Experiment-Branch: $BRANCH'
    MSG=\"eksperyment $BRANCH: rotacja poprzednich wynikow kampanii $CAMPAIGN do rotated/\$(printf '%02d' \$n)

\$MARKER\"
    if git log -1 --pretty=%B 2>/dev/null | grep -qF \"\$MARKER\"; then
      git commit --amend -m \"\$MSG\"
    else
      git commit -m \"\$MSG\"
    fi
    git push --force-with-lease origin HEAD:$BRANCH
    echo \"rotacja: poprzednie wyniki przeniesione do \$dest i wyslane\"
  else
    echo \"brak poprzednich wynikow kampanii '$CAMPAIGN' -- bez rotacji\"
  fi
"

# --- README.md kampanii: generowany PRZED pierwszym badaniem, nie dopisywany
# po fakcie (REQUIREMENTS.md, pkt "kazdy katalog kampanii ma dolaczony...") ---
log "Generuje $CAMPAIGN_RESULTS_DIR/README.md na workerze..."
if [ "$CAMPAIGN_KIND" = "rate" ]; then
  CAMPAIGN_GOAL="Ustalenie maksymalnej czestosci naplywu danych (Hz), przy ktorej algorytm detekcji QRS (Pan-Tompkins, examples/ecg/rec205/rec205-detect.rql) na tym sprzecie workera dotrzymuje budzetu czasu rzeczywistego -- REQUIREMENTS.md pkt 12-15. Celem NIE jest poprawnosc detekcji, tylko przepustowosc (pkt 13)."
else
  CAMPAIGN_GOAL="Ustalenie wplywu liczby dolaczonych klientow xqry (1..10) na proces xretractor i system, przy ustalonej czestosci naplywu -- REQUIREMENTS.md pkt 16."
fi
CAMPAIGN_README="$(cat <<EOF
# Kampania: ${CAMPAIGN}

Wygenerowano automatycznie przez \`start_supervisor.sh\` przed rozpoczeciem
pierwszego badania tej kampanii. Nie edytowac recznie -- kolejne uruchomienia
kampanii nadpisuja ten plik ta sama trescia.

- branch eksperymentu: \`${BRANCH}\`
- data przygotowania kampanii: $(date -Is)
- plik konfiguracji badan: \`examples/experiment/config/campaign_${CAMPAIGN}.csv\`

## Cel

${CAMPAIGN_GOAL}

## Struktura

Kazdy podkatalog \`study_NN/\` odpowiada jednemu wierszowi pliku konfiguracji
(jednemu badaniu) i zawiera \`results.md\`, \`state_before.md\`,
\`state_after.md\`, \`e1_probe.csv\`, \`metrics.csv\` -- patrz glowny
\`examples/experiment/README.md\` po opis kazdego z tych plikow.
EOF
)"
printf '%s\n' "$CAMPAIGN_README" | ssh_worker "$WORKER_HOST" "mkdir -p '$WORKER_REPO/$CAMPAIGN_RESULTS_DIR' && cat > '$WORKER_REPO/$CAMPAIGN_RESULTS_DIR/README.md'"

# --- Petla badan ------------------------------------------------------------
mapfile -t ROWS < <(tail -n +2 "$CONFIG_CSV")
TOTAL=${#ROWS[@]}
IDX=0
for row in "${ROWS[@]}"; do
  IDX=$((IDX + 1))
  IFS=',' read -r study_id col2 col3 col4 <<< "$row"
  if [ "$CAMPAIGN_KIND" = "rate" ]; then
    rate_hz="$col2"; clients="$col3"; samples="$col4"
  else
    rate_hz="$RATE_HZ"; clients="$col2"; samples="$col3"
  fi

  log "--- Badanie $IDX/$TOTAL (study_id=$study_id rate=${rate_hz}Hz clients=$clients samples=$samples) ---"
  if ! ssh_worker "$WORKER_HOST" \
      "'$WORKER_REPO/examples/experiment/worker/run_study.sh' '$BRANCH' '$CAMPAIGN' '$study_id' '$rate_hz' '$clients' '$samples' '$SINK'"; then
    die "Badanie $study_id nie powiodlo sie na workerze -- przerywam kampanie (worker NIE zostal zrestartowany)"
  fi

  if [ "$IDX" -lt "$TOTAL" ]; then
    log "Restart maszyny workera (pkt 4)..."
    # 'sync' przed reboot: worker trzyma system i repozytorium na karcie SD,
    # a wyniki badania sa zapisywane tuz przed restartem -- wymuszamy zrzut
    # cache stron na nosnik, zeby restart nie zostawil ich niezapisanych.
    ssh_worker "$WORKER_HOST" "sync; sudo -n reboot" || true
    wait_for_worker "$WORKER_HOST" "$REBOOT_TIMEOUT"
  fi
done

log "=== Kampania '$CAMPAIGN' zakonczona. Branch: $BRANCH ==="
log "Sprawdz wyniki w examples/experiment/results/${CAMPAIGN}/ i git log na branchu $BRANCH."
