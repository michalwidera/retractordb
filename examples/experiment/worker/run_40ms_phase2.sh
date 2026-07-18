#!/bin/bash
# Uruchamiane NA maszynie workera. Faza 2 sledztwa ~40 ms: eksperyment
# rozstrzygajacy S3 "mlock-variant" (JOURNAL.md 2026-07-18).
#
# Ustalenie z danych Fazy 0: stall ~40 ms siedzi w e2e slotu POPRZEDZAJACEGO
# serie spoznien (wake czyste, compute czyste, e2e ~42 ms) -- czyli w
# boradcast(): pierwsza emisja do swiezo zarejestrowanego klienta otwiera
# kolejke IPC (message_queue open_only, executorsm.cpp:503), a mmap segmentu
# (~3,8 MB: 3600 msg x 1 KB) pod mlockall(MCL_FUTURE) populuje i blokuje
# strony synchronicznie W WATKU FIFO.
#
# Badanie: trzy warianty mlockall (RDB_MLOCKALL z executor_rt.cpp, zmiana
# na branchu eksperymentu za zgoda pkt 23):
#   populate -- status quo (kontrola; oczekiwany spike e2e ~42 ms przy attach)
#   onfault  -- MCL_ONFAULT, blokowanie stron przy dotknieciu (kandydat poprawki;
#               oczekiwane zniknieciecie spike'u)
#   off      -- bez mlockall (diagnostyczny; nie-RT-safe)
# Kazdy wariant x REPEATS przebiegow: silnik QRS @360 Hz + 1 klient xqry
# podpinany po 2 s. Dodatkowo: polityki szeregowania watkow silnika
# (ps -eLo: FIFO tylko watek przetwarzajacy) i rozmiar segmentu kolejki
# w /dev/shm -- material dowodowy do dziennika.
#
# Usage: run_40ms_phase2.sh <branch> [samples] [repeats]   # dom. 5000 3
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
# shellcheck source=../lib/common.sh
source "$REPO_ROOT/examples/experiment/lib/common.sh"

[ $# -ge 1 ] || die "Usage: $0 <branch> [samples] [repeats]"
BRANCH="$1" SAMPLES="${2:-5000}" REPEATS="${3:-3}"
VARIANTS=(populate onfault off)

RATE_HZ=360
XR_CPU=3
BG_CPUS="0-1"
ECG_SRC="$REPO_ROOT/examples/ecg/rec205"
WORKDIR="/dev/shm/rdb-40ms/phase2"
RESULTS_DIR="$REPO_ROOT/examples/experiment/results/40ms/study_mlock-variant"
STREAM_NAME="detect_out"
RUN_TIMEOUT=$(( SAMPLES * 30 / 1000 + 120 ))

PIDS_TO_KILL=()
ORIG_GOVERNOR=""
cleanup() {
  for pid in "${PIDS_TO_KILL[@]:-}"; do [ -n "$pid" ] && kill "$pid" 2>/dev/null || true; done
  if [ -n "$ORIG_GOVERNOR" ]; then
    echo "$ORIG_GOVERNOR" | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null 2>&1 || true
    ORIG_GOVERNOR=""
  fi
  wait 2>/dev/null || true
}
trap cleanup EXIT

command -v xretractor >/dev/null || die "xretractor nie na PATH"
command -v xqry >/dev/null || die "xqry nie na PATH"
[ -f "$ECG_SRC/rec205" ] || die "Brak $ECG_SRC/rec205"

log "=== Faza 2 / S3 mlock-variant: samples=$SAMPLES repeats=$REPEATS branch=$BRANCH ==="
cd "$REPO_ROOT"
git fetch origin "$BRANCH"; git checkout "$BRANCH"; git reset --hard "origin/$BRANCH"

# Weryfikacja, ze zainstalowana binarka ma przelacznik RDB_MLOCKALL -- ochrona
# przed stara binarka. grep bezposrednio na pliku (bez potoku ze strings:
# grep -q zamyka potok po trafieniu -> SIGPIPE strings -> pod pipefail
# falszywy FAIL; zlapane przy pierwszym uruchomieniu badania).
if ! grep -q "RDB_MLOCKALL" "$(command -v xretractor)"; then
  die "Binarka xretractor bez RDB_MLOCKALL -- przebuduj z brancha: scripts/buildrdb.sh conan ninja probe && ninja install"
fi

XR_BIN="$(command -v xretractor)"
if command -v getcap >/dev/null && ! getcap "$XR_BIN" 2>/dev/null | grep -q cap_sys_nice; then
  sudo -n setcap cap_sys_nice,cap_ipc_lock+ep "$XR_BIN" 2>/dev/null || log "UWAGA: setcap nieudane"
fi

gov_file=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
if [ -r "$gov_file" ] && sudo -n true 2>/dev/null; then
  ORIG_GOVERNOR=$(cat "$gov_file")
  echo performance | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null
fi

rm -rf "$WORKDIR"; mkdir -p "$WORKDIR" "$RESULTS_DIR"
cp "$ECG_SRC/rec205" "$ECG_SRC/rec205.desc" "$ECG_SRC/bp_coef.txt" "$ECG_SRC/d_coef.txt" "$WORKDIR/"
sed "s#1/360#1/${RATE_HZ}#" "$ECG_SRC/rec205-detect.rql" > "$WORKDIR/study.rql"
printf '[scheduling]\nrt_priority = 50\n' > "$WORKDIR/study.toml"

{ echo "# Wyniki badania S3 (mlock-variant) -- sledztwo 40ms, Faza 2"
  echo; echo "- data: $(date -Is)"
  echo "- silnik: rdzen $XR_CPU, -t (FIFO 50), ${RATE_HZ} Hz, $SAMPLES probek, klient xqry po 2 s"
  echo "- warianty RDB_MLOCKALL: ${VARIANTS[*]}, po $REPEATS przebiegow"
  echo "- hipoteza: spike e2e ~42 ms przy attach znika w 'onfault' i 'off'"
  echo; echo "## uname"; echo '```'; uname -a; echo '```'
} > "$RESULTS_DIR/results.md"

for variant in "${VARIANTS[@]}"; do
  for r in $(seq 1 "$REPEATS"); do
    RDIR="$WORKDIR/${variant}_rep${r}"; mkdir -p "$RDIR"; cd "$RDIR"
    cp "$WORKDIR"/rec205 "$WORKDIR"/rec205.desc "$WORKDIR"/bp_coef.txt \
       "$WORKDIR"/d_coef.txt "$WORKDIR"/study.rql "$WORKDIR"/study.toml "$RDIR/"
    log "--- wariant=$variant rep=$r ---"

    RDB_MLOCKALL="$variant" RDB_BENCH_CSV="$RDIR/e1_probe.csv" \
      timeout "${RUN_TIMEOUT}s" taskset -c $XR_CPU xretractor study.rql -k -m "$SAMPLES" -t -g study.toml \
      > "$RDIR/xretractor.log" 2>&1 &
    XR_PID=$!; PIDS_TO_KILL+=("$XR_PID")
    sleep 2
    taskset -c $BG_CPUS xqry -s "$STREAM_NAME" -r >/dev/null 2>"$RDIR/xqry.err" &
    CLIENT_PID=$!; PIDS_TO_KILL+=("$CLIENT_PID")

    sleep 1.5  # okno po attach: polityki watkow i rozmiar segmentu kolejki
    ps -eLo pid,tid,comm,class,rtprio,psr 2>/dev/null | grep -i xretractor > "$RDIR/threads.txt" || true
    ls -l /dev/shm/brcdbr* > "$RDIR/shm_queue.txt" 2>/dev/null || true

    wait "$XR_PID" || log "xretractor $variant/rep$r: kod $?"
    kill "$CLIENT_PID" 2>/dev/null || true
    PIDS_TO_KILL=()
    cd "$WORKDIR"
  done
done

# --- Analiza zbiorcza: spike e2e przy attach + seria wake per przebieg -------
{ echo; echo "## Analiza (spike e2e przy attach; zdarzenia wake >= 5 ms po slocie 100)"
  echo
  python3 - "$WORKDIR" "${VARIANTS[@]}" <<'PYEOF'
import csv, glob, sys
workdir, variants = sys.argv[1], sys.argv[2:]
print("| wariant | rep | max e2e po slocie 100 [ms] (slot) | zdarzen wake>=5ms po slocie 100 | szczyt serii [ms] |")
print("|---|---|---|---|---|")
for v in variants:
    for path in sorted(glob.glob(f"{workdir}/{v}_rep*/e1_probe.csv")):
        rep = path.split("_rep")[1].split("/")[0]
        rows = list(csv.DictReader(open(path)))
        e2e = [(int(r["e2e_ns"]), i) for i, r in enumerate(rows) if i > 100]
        wake_ev = [int(r["wake_lag_ns"]) for i, r in enumerate(rows)
                   if i > 100 and int(r["wake_lag_ns"]) >= 5_000_000]
        mx, mxi = max(e2e) if e2e else (0, -1)
        peak = max(wake_ev)/1e6 if wake_ev else 0.0
        print(f"| {v} | {rep} | {mx/1e6:.3f} ({mxi}) | {len(wake_ev)} | {peak:.3f} |")
PYEOF
  echo
  echo "## Polityki watkow silnika (przyklad z ostatniego przebiegu; FF=FIFO, TS=CFS)"
  echo '```'
  cat "$WORKDIR/off_rep${REPEATS}/threads.txt" 2>/dev/null || echo "(brak)"
  echo '```'
  echo "## Segment kolejki klienta w /dev/shm"
  echo '```'
  cat "$WORKDIR/off_rep${REPEATS}/shm_queue.txt" 2>/dev/null || echo "(brak)"
  echo '```'
} >> "$RESULTS_DIR/results.md"

for variant in "${VARIANTS[@]}"; do
  for r in $(seq 1 "$REPEATS"); do
    DEST="$RESULTS_DIR/${variant}_rep${r}"; mkdir -p "$DEST"
    cp "$WORKDIR/${variant}_rep${r}/e1_probe.csv" \
       "$WORKDIR/${variant}_rep${r}/threads.txt" \
       "$WORKDIR/${variant}_rep${r}/shm_queue.txt" \
       "$WORKDIR/${variant}_rep${r}/xretractor.log" "$DEST/" 2>/dev/null || true
  done
done

cd "$REPO_ROOT"
git add "$RESULTS_DIR"
MARKER="Experiment-Branch: ${BRANCH}"
COMMIT_MSG="$(printf 'eksperyment %s: Faza 2 sledztwa 40ms, badanie mlock-variant\n\n%s' "$BRANCH" "$MARKER")"
if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  git commit --amend -m "$COMMIT_MSG"
else
  git commit -m "$COMMIT_MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"

rm -rf "$WORKDIR"
log "Badanie mlock-variant zakonczone i wyslane."
