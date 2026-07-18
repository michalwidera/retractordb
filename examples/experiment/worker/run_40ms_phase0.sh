#!/bin/bash
# Uruchamiane NA maszynie workera (bezposrednio lub przez SSH ze
# start_40ms_phase0.sh). Faza 0 sledztwa zdarzen ~40 ms (JOURNAL.md,
# 2026-07-18): dyskryminacja "platforma system-wide vs aktywnosc silnika"
# BEZ zmian w kodzie silnika -- wylacznie procesy pomiarowe z zewnatrz.
#
# Badania:
#   pacer-solo    (S1): shadow_pacer.py SAM na rdzeniu silnika (3), FIFO 50,
#                       dlugi przebieg. Pytanie: czy platforma bez silnika
#                       generuje zdarzenia >= 5 ms? (Baseline NumPy: nie.)
#   engine-shadow (S2): silnik QRS (konfiguracja kampanii rate: 360 Hz,
#                       1 klient xqry) na rdzeniu 3 + shadow_pacer.py na
#                       rdzeniu 2. Pytanie: czy zdarzenia silnika widzi
#                       rownoczesnie niezalezny proces na sasiednim rdzeniu?
#
# Wokol kazdego przebiegu: snapshot /proc/interrupts + /proc/softirqs
# (delta IPI/CAL na rdzeniu 3 = slad TLB shootdown / call function) oraz
# dmesg. Wyniki -> results/40ms/study_<S>/, commit amend + push jak w
# run_study.sh (marker Experiment-Branch).
#
# Usage:
#   run_40ms_phase0.sh <branch> pacer-solo    [samples]           # dom. 200000
#   run_40ms_phase0.sh <branch> engine-shadow [samples] [repeats] # dom. 20000 5
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
# shellcheck source=../lib/common.sh
source "$REPO_ROOT/examples/experiment/lib/common.sh"

[ $# -ge 2 ] || die "Usage: $0 <branch> <pacer-solo|engine-shadow> [samples] [repeats] [suffix]"
BRANCH="$1" STUDY="$2"
case "$STUDY" in
  pacer-solo)    SAMPLES="${3:-200000}"; REPEATS=1 ;;
  engine-shadow) SAMPLES="${3:-20000}";  REPEATS="${4:-5}" ;;
  *) die "study musi byc 'pacer-solo' albo 'engine-shadow'" ;;
esac
# Sufiks katalogu wynikow (np. "-fix" dla walidacji poprawki Fazy 3): powtorka
# badania NIE moze nadpisac wynikow sprzed poprawki -- to material dowodowy.
SUFFIX="${5:-}"

RATE_HZ=360
XR_CPU=3      # rdzen silnika / pacera-solo (isolcpus)
SHADOW_CPU=2  # rdzen cienia w engine-shadow
BG_CPUS="0-1" # klienci xqry i sampler metryk

CONFIG_DIR="$REPO_ROOT/examples/experiment/config"
ECG_SRC="$REPO_ROOT/examples/ecg/rec205"
WORKDIR="/dev/shm/rdb-40ms/${STUDY}"
RESULTS_DIR="$REPO_ROOT/examples/experiment/results/40ms/study_${STUDY}${SUFFIX}"
STREAM_NAME="detect_out"

PIDS_TO_KILL=()
ORIG_GOVERNOR=""

mono_ns() { python3 -c 'import time; print(time.clock_gettime_ns(time.CLOCK_MONOTONIC))'; }

cleanup() {
  for pid in "${PIDS_TO_KILL[@]:-}"; do [ -n "$pid" ] && kill "$pid" 2>/dev/null || true; done
  if [ -n "$ORIG_GOVERNOR" ]; then
    echo "$ORIG_GOVERNOR" | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null 2>&1 || true
    ORIG_GOVERNOR=""
  fi
  wait 2>/dev/null || true
}
trap cleanup EXIT

set_performance_governor() {
  local gov_file=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  [ -r "$gov_file" ] || { log "UWAGA: brak cpufreq -- pomijam governor"; return; }
  if sudo -n true 2>/dev/null; then
    ORIG_GOVERNOR=$(cat "$gov_file")
    echo performance | sudo -n tee /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_governor >/dev/null
    log "Governor CPU: performance (bylo: $ORIG_GOVERNOR)"
  else
    log "UWAGA: brak bezhaslowego sudo -- governor bez zmian"
  fi
}

snapshot_irq() {  # /proc/interrupts + /proc/softirqs w jednym pliku
  { echo "=== /proc/interrupts ==="; cat /proc/interrupts
    echo; echo "=== /proc/softirqs ==="; cat /proc/softirqs; } > "$1"
}

snapshot_state() {
  local label="$1" out="$2"
  { echo "# Stan maszyny -- $label (sledztwo 40ms, badanie $STUDY)"
    echo; echo "- data: $(date -Is)"
    echo "- parametry: samples=$SAMPLES repeats=$REPEATS rate=${RATE_HZ}Hz"
    echo; echo '## uname'; echo '```'; uname -a; echo '```'
    echo '## Kernel cmdline'; echo '```'; cat /proc/cmdline; echo '```'
    echo '## Load / pamiec'; echo '```'; cat /proc/loadavg; free -h; echo '```'
    echo '## Temperatura'; echo '```'
    for z in /sys/class/thermal/thermal_zone*/temp; do
      [ -r "$z" ] && printf '%s: %d m°C\n' "$z" "$(cat "$z")"
    done
    echo '```'
    echo '## Governor per rdzen'; echo '```'
    for c in /sys/devices/system/cpu/cpu[0-9]*/cpufreq; do
      [ -r "$c/scaling_governor" ] || continue
      printf '%s: %s @ %s kHz\n' "$(basename "$(dirname "$c")")" \
        "$(cat "$c/scaling_governor")" "$(cat "$c/scaling_cur_freq")"
    done
    command -v vcgencmd >/dev/null && echo "throttled: $(vcgencmd get_throttled)"
    echo '```'
  } > "$out"
}

# Nadanie FIFO 50 pacerowi w oknie settle (protokol z run_numpy_baseline.sh:
# proces drukuje "PID <n>", chrt z zewnatrz nie zmienia wlasnosci plikow).
grant_fifo_from_log() {
  local logfile="$1" pid=""
  for _ in $(seq 1 20); do
    pid=$(grep -m1 '^PID ' "$logfile" 2>/dev/null | awk '{print $2}') && [ -n "$pid" ] && break
    sleep 0.1
  done
  if [ -n "$pid" ] && sudo -n chrt -f -p 50 "$pid" 2>/dev/null; then
    log "SCHED_FIFO 50 nadane pacerowi (pid $pid)"
  else
    log "UWAGA: chrt na pacera nieudane -- pacer biegnie bez FIFO (miekka degradacja)"
  fi
}

log "=== Faza 0 / badanie $STUDY: samples=$SAMPLES repeats=$REPEATS branch=$BRANCH ==="

cd "$REPO_ROOT"
git fetch origin "$BRANCH"
git checkout "$BRANCH"
git reset --hard "origin/$BRANCH"

rm -rf "$WORKDIR"; mkdir -p "$WORKDIR" "$RESULTS_DIR"
set_performance_governor
snapshot_state "PRZED badaniem" "$WORKDIR/state_before.md"

if [ "$STUDY" = "pacer-solo" ]; then
  # --- S1: sam pacer na rdzeniu silnika --------------------------------------
  snapshot_irq "$WORKDIR/irq_before.txt"
  taskset -c $XR_CPU python3 "$CONFIG_DIR/shadow_pacer.py" \
    --rate $RATE_HZ --samples "$SAMPLES" --tag "S1-core${XR_CPU}" \
    --out "$WORKDIR/pacer_solo.csv" > "$WORKDIR/pacer_solo.log" 2>&1 &
  PACER_PID=$!; PIDS_TO_KILL+=("$PACER_PID")
  grant_fifo_from_log "$WORKDIR/pacer_solo.log"
  log "Pacer-solo w toku (~$((SAMPLES / RATE_HZ / 60)) min)..."
  wait "$PACER_PID"; PIDS_TO_KILL=()
  snapshot_irq "$WORKDIR/irq_after.txt"

  { echo "# Wyniki badania S1 (pacer-solo) -- sledztwo 40ms, Faza 0"
    echo; echo "- data: $(date -Is)"
    echo "- pacer: rdzen $XR_CPU (isolcpus), FIFO 50, ${RATE_HZ} Hz, $SAMPLES slotow, zero pracy w slocie"
    echo "- pytanie: czy platforma BEZ silnika generuje zdarzenia >= 5 ms?"
    echo
    python3 "$CONFIG_DIR/analyze_40ms.py" "pacer-solo=$WORKDIR/pacer_solo.csv"
    echo; echo "## Log pacera"; echo '```'; cat "$WORKDIR/pacer_solo.log"; echo '```'
  } > "$RESULTS_DIR/results.md"
  gzip -c "$WORKDIR/pacer_solo.csv" > "$RESULTS_DIR/pacer_solo.csv.gz"

else
  # --- S2: silnik + cien, REPEATS powtorzen ----------------------------------
  command -v xretractor >/dev/null || die "xretractor nie na PATH (build z -DRDB_BENCH_PROBE=ON + ninja install)"
  command -v xqry >/dev/null || die "xqry nie na PATH"
  [ -f "$ECG_SRC/rec205" ] || die "Brak $ECG_SRC/rec205 (examples/ecg/build.sh)"

  XR_BIN="$(command -v xretractor)"
  if command -v getcap >/dev/null && ! getcap "$XR_BIN" 2>/dev/null | grep -q cap_sys_nice; then
    sudo -n setcap cap_sys_nice,cap_ipc_lock+ep "$XR_BIN" 2>/dev/null \
      && log "Nadano capabilities RT na $XR_BIN" \
      || log "UWAGA: setcap nieudane -- -t bez SCHED_FIFO (miekka degradacja)"
  fi

  cp "$ECG_SRC/rec205" "$ECG_SRC/rec205.desc" "$ECG_SRC/bp_coef.txt" "$ECG_SRC/d_coef.txt" "$WORKDIR/"
  sed "s#1/360#1/${RATE_HZ}#" "$ECG_SRC/rec205-detect.rql" > "$WORKDIR/study.rql"
  printf '[scheduling]\nrt_priority = 50\n' > "$WORKDIR/study.toml"
  RUN_TIMEOUT=$(( SAMPLES * 30 / 1000 + 120 ))
  PACER_SAMPLES=$(( SAMPLES + 30 * RATE_HZ ))  # cien z zapasem; ubijany SIGTERM po silniku

  { echo "# Wyniki badania S2 (engine-shadow) -- sledztwo 40ms, Faza 0"
    echo; echo "- data: $(date -Is)"
    echo "- silnik: rdzen $XR_CPU, FIFO 50 (-t), ${RATE_HZ} Hz, $SAMPLES probek, 1 klient xqry (sink=null, rdzenie $BG_CPUS)"
    echo "- cien: shadow_pacer.py na rdzeniu $SHADOW_CPU, FIFO 50, zero pracy"
    echo "- powtorzen: $REPEATS; miedzy powtorzeniami bez rebootu (reboot rozdziela badania, nie powtorzenia)"
    echo "- pytanie: czy zdarzenia >= 5 ms u silnika widzi rownoczesnie niezalezny proces na sasiednim rdzeniu?"
  } > "$RESULTS_DIR/results.md"

  for r in $(seq 1 "$REPEATS"); do
    RDIR="$WORKDIR/rep${r}"; mkdir -p "$RDIR"; cd "$RDIR"
    cp "$WORKDIR"/rec205 "$WORKDIR"/rec205.desc "$WORKDIR"/bp_coef.txt "$WORKDIR"/d_coef.txt \
       "$WORKDIR"/study.rql "$WORKDIR"/study.toml "$RDIR/"
    log "--- Powtorzenie $r/$REPEATS ---"
    snapshot_irq "$RDIR/irq_before.txt"

    taskset -c $SHADOW_CPU python3 "$CONFIG_DIR/shadow_pacer.py" \
      --rate $RATE_HZ --samples "$PACER_SAMPLES" --tag "S2-core${SHADOW_CPU}-rep${r}" \
      --out "$RDIR/shadow.csv" > "$RDIR/shadow.log" 2>&1 &
    SHADOW_PID=$!; PIDS_TO_KILL+=("$SHADOW_PID")
    grant_fifo_from_log "$RDIR/shadow.log"

    LAUNCH_MONO=$(mono_ns)
    RDB_BENCH_CSV="$RDIR/e1_probe.csv" \
      timeout "${RUN_TIMEOUT}s" taskset -c $XR_CPU xretractor study.rql -k -m "$SAMPLES" -t -g study.toml \
      > "$RDIR/xretractor.log" 2>&1 &
    XR_PID=$!; PIDS_TO_KILL+=("$XR_PID")
    sleep 2  # start serwera IPC przed klientem
    taskset -c $BG_CPUS xqry -s "$STREAM_NAME" -r >/dev/null 2>"$RDIR/xqry.err" &
    CLIENT_PID=$!; PIDS_TO_KILL+=("$CLIENT_PID")

    wait "$XR_PID" || log "xretractor rep$r: kod $? (sprawdz logi)"
    EXIT_MONO=$(mono_ns)
    kill "$CLIENT_PID" 2>/dev/null || true
    kill -TERM "$SHADOW_PID" 2>/dev/null || true   # pacer dopisze partial dump
    wait "$SHADOW_PID" 2>/dev/null || true
    PIDS_TO_KILL=()
    snapshot_irq "$RDIR/irq_after.txt"
    printf 'launch_mono_ns=%s\nexit_mono_ns=%s\n' "$LAUNCH_MONO" "$EXIT_MONO" > "$RDIR/meta.txt"

    { echo; echo "## Powtorzenie $r"
      echo; echo "- launch_mono_ns=$LAUNCH_MONO exit_mono_ns=$EXIT_MONO"
      echo
      if [ -s "$RDIR/e1_probe.csv" ]; then
        python3 "$CONFIG_DIR/analyze_40ms.py" \
          "engine=$RDIR/e1_probe.csv" "shadow=$RDIR/shadow.csv" \
          --rate $RATE_HZ \
          --engine-launch-mono-ns "$LAUNCH_MONO" --engine-exit-mono-ns "$EXIT_MONO"
      else
        echo "BLAD: e1_probe.csv puste -- binarka bez -DRDB_BENCH_PROBE=ON?"
      fi
      echo; echo "### Delta IPI (irq_before -> irq_after, pelne pliki w repo)"
      echo '```'
      diff "$RDIR/irq_before.txt" "$RDIR/irq_after.txt" | grep -E 'IPI|CAL|Function call' || echo "(brak roznic w liniach IPI)"
      echo '```'
    } >> "$RESULTS_DIR/results.md"

    DEST="$RESULTS_DIR/rep${r}"; mkdir -p "$DEST"
    cp "$RDIR/e1_probe.csv" "$DEST/" 2>/dev/null || true
    gzip -c "$RDIR/shadow.csv" > "$DEST/shadow.csv.gz" 2>/dev/null || true
    cp "$RDIR/irq_before.txt" "$RDIR/irq_after.txt" "$RDIR/meta.txt" \
       "$RDIR/xretractor.log" "$DEST/" 2>/dev/null || true
    cd "$WORKDIR"
  done
fi

snapshot_state "PO badaniu" "$WORKDIR/state_after.md"
cp "$WORKDIR/state_before.md" "$WORKDIR/state_after.md" "$RESULTS_DIR/" 2>/dev/null || true
[ -f "$WORKDIR/irq_before.txt" ] && cp "$WORKDIR/irq_before.txt" "$WORKDIR/irq_after.txt" "$RESULTS_DIR/" 2>/dev/null || true
sudo -n dmesg -T 2>/dev/null | tail -300 > "$RESULTS_DIR/dmesg_tail.txt" || true

# --- Commit + push (konwencja run_study.sh: amend per branch eksperymentu) ---
cd "$REPO_ROOT"
git add "$RESULTS_DIR"
MARKER="Experiment-Branch: ${BRANCH}"
COMMIT_MSG="$(printf 'eksperyment %s: Faza 0 sledztwa 40ms, badanie %s\n\n%s' \
  "$BRANCH" "$STUDY" "$MARKER")"
if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  git commit --amend -m "$COMMIT_MSG"
else
  git commit -m "$COMMIT_MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"

rm -rf "$WORKDIR"
log "Badanie $STUDY zakonczone i wyslane."
