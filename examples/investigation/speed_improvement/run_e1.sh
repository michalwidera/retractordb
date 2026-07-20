#!/usr/bin/env bash
# =============================================================================
# run_e1.sh — pomiar baseline E1 (rdzeń obliczeń processRows) dla inwestygacji
#             speed_improvement.
#
# Uruchamia xretractor (kompilacja z sondą RDB_BENCH_PROBE) na stałym workloadzie
# ECG rec205-detect, 5 przebiegów unpaced (-k -m 650000, bez -t), zbiera CSV
# sondy, przepuszcza przez e1_stats.py i agreguje p50/p99 z 5 przebiegów.
#
# WYMAGANIA: binarka zbudowana z sondą:  scripts/buildrdb.sh probe
#            (xretractor w build/Release/ — patrz BIN poniżej).
#
# UŻYCIE (z korzenia repo):
#   examples/investigation/speed_improvement/run_e1.sh <label>
#   # <label> np. baseline, opt-copy, ...  -> wynik: results/<label>.md
# =============================================================================
set -euo pipefail

LABEL="${1:?podaj etykiete, np. baseline}"
RUNS="${RUNS:-5}"
# 20k interwałów wystarcza dla stabilnej mediany/p99 (p50 powtarzalne < 1%);
# pełne 650k to ~30 min/przebieg (pętla śpi ~2,78 ms/slot bez -t) — zbędne dla p50.
SAMPLES="${SAMPLES:-20000}"

REPO="$(git rev-parse --show-toplevel)"
BIN="${BIN:-$REPO/build/Release/src/retractor/xretractor}"
RQLDIR="$REPO/examples/ecg/rec205"
RQL="$RQLDIR/rec205-detect.rql"
STATS="$REPO/examples/ecg/e1_stats.py"
OUTDIR="$REPO/examples/investigation/speed_improvement/results"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

[ -x "$BIN" ] || { echo "BRAK binarki z sonda: $BIN — zbuduj: scripts/buildrdb.sh probe"; exit 1; }
mkdir -p "$OUTDIR"

# taskset na 1 rdzen jesli dostepny (redukcja szumu migracji)
PIN=""
if command -v taskset >/dev/null 2>&1; then PIN="taskset -c 3"; fi

p50s=(); p99s=(); thrs=()
echo "== E1 $LABEL: $RUNS przebiegow, $SAMPLES probek, workload rec205-detect =="
cd "$RQLDIR"   # RQL czyta pliki danych z CWD (rec205, bp_coef.txt, d_coef.txt)
for i in $(seq 1 "$RUNS"); do
  csv="$TMPDIR/e1_$i.csv"
  RDB_BENCH_CSV="$csv" $PIN "$BIN" "$RQL" -k -m "$SAMPLES" >/dev/null 2>&1 || true
  out="$(python3 "$STATS" "$csv" 2>/dev/null)"
  p50="$(echo "$out" | awk '/E1: rdzen|E1: rdze/{f=1} f&&/mediana/{print $3; exit}')"
  p99="$(echo "$out" | awk '/E1: rdzen|E1: rdze/{f=1} f&&/p99/{print $3; exit}')"
  thr="$(echo "$out" | awk '/przepustowo/{print $3; exit}')"
  echo "  run $i: p50=${p50} us  p99=${p99} us  thr=${thr} prob/s"
  p50s+=("$p50"); p99s+=("$p99")
done

med() { printf '%s\n' "$@" | sort -n | awk '{a[NR]=$1} END{print (NR%2)?a[(NR+1)/2]:(a[NR/2]+a[NR/2+1])/2}'; }
mn()  { printf '%s\n' "$@" | sort -n | head -1; }
mx()  { printf '%s\n' "$@" | sort -n | tail -1; }

P50MED="$(med "${p50s[@]}")"; P50MIN="$(mn "${p50s[@]}")"; P50MAX="$(mx "${p50s[@]}")"
P99MED="$(med "${p99s[@]}")"

{
  echo "# E1 wynik: $LABEL"
  echo
  echo "- data: $(date -Iseconds)"
  echo "- binarka: $BIN"
  echo "- workload: rec205-detect.rql, -m $SAMPLES, unpaced, $RUNS przebiegow"
  echo "- pin: ${PIN:-brak}"
  echo
  echo "| wskaznik | wartosc |"
  echo "|---|---|"
  echo "| **p50 compute (mediana z $RUNS)** | **$P50MED us** |"
  echo "| p50 rozrzut (min-max) | $P50MIN - $P50MAX us |"
  echo "| p99 compute (mediana z $RUNS) | $P99MED us |"
  echo
  echo "przebiegi p50: ${p50s[*]}"
  echo "przebiegi p99: ${p99s[*]}"
} | tee "$OUTDIR/$LABEL.md"

echo "== zapisano: $OUTDIR/$LABEL.md =="
