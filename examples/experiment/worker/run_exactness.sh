#!/bin/bash
# Kampania exactness/replay (REQUIREMENTS.md, "Czesc dla 7.6"; cel: sekcja 7.6
# main-debs.tex). Uruchamiane NA workerze. Trzy badania:
#   A (replay)    — dwa identyczne przebiegi potoku QRS z zapisem WSZYSTKICH
#                   strumieni (config/exactness-replay.rql); porownanie
#                   artefaktow co do bitu: dane + .desc identyczne, .meta
#                   identyczne po odcieciu 8-bajtowego znacznika czasu
#                   utworzenia w naglowku.
#   B (roundtrip) — tozsamosc okrezna cor:exact na kanalach EKG: faza 1 zapis
#                   artefaktow a/b, faza 2 DECLARE z artefaktow -> c=a#b ->
#                   a2=c&1/360, b2=c%1/360; oczekiwanie: c = przeplot dokladny,
#                   a2[1:] == a i b2 == b co do bitu (Theta opozniona o slot).
#   C (float)     — referencja zmiennoprzecinkowa (config/resample_roundtrip.py):
#                   round-trip 360->720->360 Hz, norma bledu w funkcji N.
# Pomiar dotyczy ROWNOSCI BITOWEJ, nie czasu — przebiegi unpaced (bez -t);
# dyscyplina srodowiska (governor performance, taskset -c 3, /dev/shm)
# utrzymana dla spojnosci metodycznej z kampaniami dni 1-2.
#
# Usage: run_exactness.sh <branch> [samples=20000]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
source "$REPO_ROOT/examples/experiment/lib/common.sh"

BRANCH="${1:?Usage: run_exactness.sh <branch> [samples]}"
SAMPLES="${2:-20000}"

# /usr/bin/python3 jawnie: venv conana (~/.venv) przeslania systemowe
# numpy/scipy (apt), ktorych uzywaja badania B i C.
PYBIN=/usr/bin/python3
"$PYBIN" -c "import numpy, scipy" 2>/dev/null || die "brak numpy/scipy w $PYBIN"

[ -d /dev/shm ] || die "/dev/shm niedostepny (pkt 17)"
WORKDIR="/dev/shm/exactness"
RESULTS_DIR="$REPO_ROOT/examples/experiment/results/exactness"
XR_CPU=3
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

log "=== Kampania exactness: branch=$BRANCH samples=$SAMPLES ==="

cd "$REPO_ROOT"
git fetch origin "$BRANCH" --quiet
git checkout "$BRANCH" >/dev/null 2>&1
git reset --hard "origin/$BRANCH" >/dev/null

# Rebuild silnika (fix #6 musi byc w binarce) + idempotentny setcap
# (capabilities znikaja po kazdym ninja install; tu -t nieuzywane, ale
# utrzymujemy stan workera spojny dla kolejnych kampanii).
log "Rebuild silnika (Release+probe)..."
export PATH="$HOME/.local/bin:$PATH"
[ -f "$HOME/.venv/bin/activate" ] && source "$HOME/.venv/bin/activate"
scripts/buildrdb.sh conan ninja probe >/dev/null 2>&1 || die "rebuild nieudany"
(cd build/Release && ninja install >/dev/null) || die "ninja install nieudany"
deactivate 2>/dev/null || true
XR_BIN="$HOME/.local/bin/xretractor"
sudo -n setcap cap_sys_nice,cap_ipc_lock+ep "$XR_BIN" 2>/dev/null || true
XR_VERSION="$("$XR_BIN" --help 2>/dev/null | grep -E '^Branch:' || echo 'Branch: nieznany')"

ECG_SRC="$REPO_ROOT/examples/ecg/rec205"
[ -f "$ECG_SRC/rec205" ] || die "Brak $ECG_SRC/rec205"
CONFIG="$REPO_ROOT/examples/experiment/config"

rm -rf "$WORKDIR" && mkdir -p "$WORKDIR"
mkdir -p "$RESULTS_DIR"

snapshot_state() {
  local label="$1" out="$2"
  {
    echo "# Stan maszyny -- $label"
    echo
    echo "- data: $(date -Is)"
    echo "- badanie: campaign=exactness samples=$SAMPLES"
    echo "- binarka: $XR_VERSION"
    echo
    echo '## uname'
    echo '```'; uname -a; echo '```'
    echo '## Load average'
    echo '```'; cat /proc/loadavg; echo '```'
    echo '## Kernel cmdline'
    echo '```'; cat /proc/cmdline; echo '```'
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
  log "Governor: performance (bylo: $ORIG_GOVERNOR)"
fi

snapshot_state "PRZED badaniem" "$WORKDIR/state_before.md"

sample_metrics() {
  local out="$1"
  echo "ts_epoch,load1,mem_used_kb,temp_mC" > "$out"
  while true; do
    local temp=""
    for z in /sys/class/thermal/thermal_zone*/temp; do
      [ -r "$z" ] && { temp=$(cat "$z"); break; }
    done
    echo "$(date +%s),$(cut -d' ' -f1 /proc/loadavg),$(awk '/MemTotal/{t=$2} /MemAvailable/{a=$2} END{print t-a}' /proc/meminfo),${temp:-NA}" >> "$out"
    sleep 1
  done
}
sample_metrics "$WORKDIR/metrics.csv" &
METRICS_PID=$!

# ---------- Badanie A: replay (dwa identyczne przebiegi) ----------
log "Badanie A: replay x2 (${SAMPLES} probek)..."
for run in 1 2; do
  D="$WORKDIR/runA$run"
  mkdir -p "$D"
  cp "$ECG_SRC/rec205" "$ECG_SRC/bp_coef.txt" "$ECG_SRC/d_coef.txt" "$D/"
  cp "$CONFIG/exactness-replay.rql" "$D/"
  (cd "$D" && taskset -c $XR_CPU "$XR_BIN" exactness-replay.rql -r -k -m "$SAMPLES" > run.log 2>&1) \
    || die "replay run$run nieudany"
done

REPLAY_VERDICT="OK"
REPLAY_REPORT="$WORKDIR/replay_compare.txt"
: > "$REPLAY_REPORT"
(cd "$WORKDIR/runA1" && sha256sum $(ls | grep -vE '^(rec205|bp_coef|d_coef|exactness-replay|run\.log)') | sort -k2 > "$WORKDIR/replay_hashes_run1.txt")
(cd "$WORKDIR/runA2" && sha256sum $(ls | grep -vE '^(rec205|bp_coef|d_coef|exactness-replay|run\.log)') | sort -k2 > "$WORKDIR/replay_hashes_run2.txt")
while read -r _ f; do
  case "$f" in
    *.meta)
      if cmp -s "$WORKDIR/runA1/$f" "$WORKDIR/runA2/$f"; then
        echo "IDENTYCZNY          $f" >> "$REPLAY_REPORT"
      elif cmp -s <(tail -c +9 "$WORKDIR/runA1/$f") <(tail -c +9 "$WORKDIR/runA2/$f"); then
        echo "IDENT-PO-TIMESTAMP  $f" >> "$REPLAY_REPORT"
      else
        echo "ROZNY               $f" >> "$REPLAY_REPORT"; REPLAY_VERDICT="FAIL"
      fi ;;
    *)
      if cmp -s "$WORKDIR/runA1/$f" "$WORKDIR/runA2/$f"; then
        echo "IDENTYCZNY          $f" >> "$REPLAY_REPORT"
      else
        echo "ROZNY               $f" >> "$REPLAY_REPORT"; REPLAY_VERDICT="FAIL"
      fi ;;
  esac
done < "$WORKDIR/replay_hashes_run1.txt"
log "Badanie A: $REPLAY_VERDICT"

# ---------- Badanie B: round-trip przeplotu/rozplotu ----------
log "Badanie B: round-trip (${SAMPLES} probek)..."
D="$WORKDIR/runB"
mkdir -p "$D"
cp "$ECG_SRC/rec205" "$D/"
cp "$CONFIG/exactness-roundtrip-write.rql" "$CONFIG/exactness-roundtrip-read.rql" "$D/"
(cd "$D" && taskset -c $XR_CPU "$XR_BIN" exactness-roundtrip-write.rql -r -k -m "$SAMPLES" > write.log 2>&1) \
  || die "roundtrip faza 1 nieudana"
(cd "$D" && mv a a_data && mv b b_data && rm -f a.desc a.meta a.shadow b.desc b.meta b.shadow)
(cd "$D" && taskset -c $XR_CPU "$XR_BIN" exactness-roundtrip-read.rql -r -k -m $((2 * SAMPLES - 4)) > read.log 2>&1) \
  || die "roundtrip faza 2 nieudana"

ROUNDTRIP_REPORT="$WORKDIR/roundtrip_compare.txt"
if (cd "$D" && "$PYBIN" - > "$ROUNDTRIP_REPORT" 2>&1) <<'PYEOF'
import sys
import numpy as np

a  = np.fromfile("a_data", dtype="<i4")
b  = np.fromfile("b_data", dtype="<i4")
c  = np.fromfile("c",      dtype="<i4")
a2 = np.fromfile("a2",     dtype="<i4")
b2 = np.fromfile("b2",     dtype="<i4")

ok = True
n = len(c) // 2
w_b = bool(np.array_equal(c[0::2][:n], b[:n]))          # c[2i]   == b_i
w_a = bool(np.array_equal(c[1::2][:len(c) - n], a[:len(c) - n]))  # c[2i+1] == a_i
ok &= w_a and w_b
print(f"c: n={len(c)} przeplot b0,a0,b1,a1,...: parzyste==b:{w_b} nieparzyste==a:{w_a}")

r0 = bool(a2[0] == 0)
wa2 = bool(np.array_equal(a2[1:], a[:len(a2) - 1]))
ok &= r0 and wa2
print(f"a2: n={len(a2)} rekord0_null_placeholder:{r0} a2[1:]==a[:n-1]:{wa2}")

wb2 = bool(np.array_equal(b2, b[:len(b2)]))
ok &= wb2
print(f"b2: n={len(b2)} b2==b[:n]:{wb2}")

for name, arr in (("a2", a2), ("b2", b2)):
    if len(arr) < 19000:
        print(f"UWAGA: {name} ma tylko {len(arr)} rekordow")
        ok = False

print("VERDICT:", "OK" if ok else "FAIL")
sys.exit(0 if ok else 1)
PYEOF
then ROUNDTRIP_VERDICT="OK"; else ROUNDTRIP_VERDICT="FAIL"; fi
log "Badanie B: $ROUNDTRIP_VERDICT"

# ---------- Badanie C: referencja float ----------
log "Badanie C: referencja zmiennoprzecinkowa (resample round-trip)..."
(cd "$WORKDIR" && cp "$ECG_SRC/rec205" . && taskset -c $XR_CPU "$PYBIN" "$CONFIG/resample_roundtrip.py" \
  --rec205 rec205 --out float_roundtrip.csv > float_stdout.log 2>&1) || die "badanie C nieudane"

kill "$METRICS_PID" 2>/dev/null || true
METRICS_PID=""
snapshot_state "PO badaniu" "$WORKDIR/state_after.md"

# ---------- results.md ----------
{
  echo "# Wyniki kampanii exactness (7.6: Exactness and Replay Stability)"
  echo
  echo "- data: $(date -Is)"
  echo "- branch: $BRANCH, binarka: $XR_VERSION"
  echo "- probki: $SAMPLES @ 360 Hz (rec205), przebiegi unpaced (rownosc bitowa, nie czas)"
  echo "- srodowisko: governor performance, taskset -c $XR_CPU, /dev/shm, isolcpus wg cmdline"
  echo
  echo "## Badanie A — replay: dwa identyczne przebiegi potoku QRS (17 strumieni)"
  echo
  echo "Werdykt: **$REPLAY_VERDICT**"
  echo
  echo '```'
  cat "$REPLAY_REPORT"
  echo '```'
  echo
  echo "## Badanie B — tozsamosc okrezna przeplotu/rozplotu (cor:exact)"
  echo
  echo "Werdykt: **$ROUNDTRIP_VERDICT**"
  echo
  echo '```'
  cat "$ROUNDTRIP_REPORT"
  echo '```'
  echo
  echo "## Badanie C — referencja float (scipy resample round-trip 360->720->360 Hz)"
  echo
  echo '```'
  cat "$WORKDIR/float_stdout.log"
  echo '```'
  echo
  echo "## Metryki systemowe"
  echo '```'
  awk -F, 'NR>1{load+=$2; mem+=$3; if($4!="NA"){temp+=$4; n++}; c++}
           END{if(c>0) printf "srednie load1=%.2f mem_used_kb=%.0f", load/c, mem/c;
               if(n>0) printf " temp_mC=%.0f", temp/n; print ""}' "$WORKDIR/metrics.csv"
  echo '```'
  echo
  echo "## Pliki w tym katalogu"
  echo "- replay_hashes_run1/2.txt — sha256 wszystkich artefaktow obu przebiegow"
  echo "- replay_compare.txt — porownanie per plik (dane/.desc/.shadow bitowo; .meta po odcieciu 8 B naglowka)"
  echo "- roundtrip_compare.txt — porownania bitowe c/a2/b2 z a/b"
  echo "- float_roundtrip.csv — normy bledu float w funkcji N (metody poly i fft)"
  echo "- state_before.md / state_after.md, metrics.csv — stan i probkowanie maszyny"
} > "$RESULTS_DIR/results.md"

cp "$WORKDIR/replay_hashes_run1.txt" "$WORKDIR/replay_hashes_run2.txt" \
   "$WORKDIR/replay_compare.txt" "$WORKDIR/roundtrip_compare.txt" \
   "$WORKDIR/float_roundtrip.csv" "$WORKDIR/metrics.csv" \
   "$WORKDIR/state_before.md" "$WORKDIR/state_after.md" \
   "$RESULTS_DIR/" 2>/dev/null || true

# --- Commit + push zgodnie z konwencja eksperymentu (amend na wspolnym commicie)
cd "$REPO_ROOT"
git add "$RESULTS_DIR"
MARKER="Experiment-Branch: ${BRANCH}"
MSG="eksperyment ${BRANCH}: kampania exactness (7.6)

${MARKER}"
if git log -1 --pretty=%B 2>/dev/null | grep -qF "$MARKER"; then
  git commit --amend -m "$MSG"
else
  git commit -m "$MSG"
fi
git push --force-with-lease origin "HEAD:${BRANCH}"
rm -rf "$WORKDIR"
log "Kampania exactness zakonczona: A=$REPLAY_VERDICT B=$ROUNDTRIP_VERDICT"
