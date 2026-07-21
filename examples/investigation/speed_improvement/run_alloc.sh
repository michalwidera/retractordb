#!/usr/bin/env bash
# =============================================================================
# run_alloc.sh — deterministyczny pomiar ALOKACJI STERTY na interwał dla
#                inwestygacji speed_improvement (kandydaci K1/K2 — churn std::any).
#
# Analogicznie do RDB_COPY_COUNTER: binarka z licznikiem (RDB_ALLOC_COUNTER)
# raportuje na stderr sumę alokacji/bajtów/zwolnień CAŁEGO procesu przy wyjściu.
# Pojedynczy total miesza narzut startowy (ANTLR, IPC) z kosztem pętli, więc
# używamy METODY RÓŻNICY dwóch przebiegów o różnej liczbie interwałów N1 < N2:
#
#     alokacje/interwał = (alloc(N2) − alloc(N1)) / (N2 − N1)
#
# Stały narzut startowy się skraca; zostaje DETERMINISTYCZNY koszt jednego
# interwału (szum startowy ±kilkanaście alokacji kasuje się w różnicy — sprawdzone).
# Odporne na szum WSL2 (nie mierzymy czasu, tylko liczby całkowite).
#
# WYMAGANIA: binarka z licznikiem:
#   scripts/buildrdb.sh probe
#   ( cd build/Release && cmake -DRDB_ALLOC_COUNTER=ON . && ninja xretractor )
#
# UŻYCIE (z korzenia repo):
#   examples/investigation/speed_improvement/run_alloc.sh <label> [N1] [N2]
#   # np.  run_alloc.sh baseline        (domyślnie N1=1000, N2=6000)
#   #      run_alloc.sh k1-any-removed
#   # wynik: results/<label>-alloc.md
# =============================================================================
set -euo pipefail

LABEL="${1:?podaj etykiete, np. baseline}"
N1="${2:-1000}"
N2="${3:-6000}"
[ "$N2" -gt "$N1" ] || { echo "N2 ($N2) musi byc > N1 ($N1)"; exit 1; }
DELTA=$((N2 - N1))

REPO="$(git rev-parse --show-toplevel)"
BIN="${BIN:-$REPO/build/Release/src/retractor/xretractor}"
RQLDIR="$REPO/examples/ecg/rec205"
RQL="$RQLDIR/rec205-detect.rql"
OUTDIR="$REPO/examples/investigation/speed_improvement/results"

[ -x "$BIN" ] || { echo "BRAK binarki: $BIN — zbuduj probe + -DRDB_ALLOC_COUNTER=ON"; exit 1; }
mkdir -p "$OUTDIR"

PIN=""
if command -v taskset >/dev/null 2>&1; then PIN="taskset -c 3"; fi

# Sprawdzenie, ze licznik jest wkompilowany (inaczej brak raportu -> falszywe zera).
probe_err="$(mktemp)"; trap 'rm -f "$probe_err"' EXIT
cd "$RQLDIR"   # RQL czyta pliki danych z CWD

run_alloc() {  # $1 = liczba interwalow; echo "allocs bytes frees"
  local n="$1" err
  err="$(mktemp)"
  RDB_BENCH_CSV=/dev/null $PIN "$BIN" "$RQL" -k -m "$n" >/dev/null 2>"$err" || true
  local line; line="$(grep 'RDB_ALLOC_COUNTER' "$err" || true)"
  rm -f "$err"
  [ -n "$line" ] || { echo "BRAK raportu RDB_ALLOC_COUNTER — binarka bez -DRDB_ALLOC_COUNTER=ON" >&2; exit 1; }
  local a b f
  a="$(echo "$line" | grep -o 'allocs total: [0-9]*' | grep -o '[0-9]*')"
  b="$(echo "$line" | grep -o 'bytes total: [0-9]*'  | grep -o '[0-9]*')"
  f="$(echo "$line" | grep -o 'frees total: [0-9]*'  | grep -o '[0-9]*')"
  echo "$a $b $f"
}

echo "== alloc $LABEL: N1=$N1 N2=$N2 (delta=$DELTA), workload rec205-detect =="
read -r A1 B1 F1 < <(run_alloc "$N1"); echo "  N1=$N1: allocs=$A1 bytes=$B1 frees=$F1"
read -r A2 B2 F2 < <(run_alloc "$N2"); echo "  N2=$N2: allocs=$A2 bytes=$B2 frees=$F2"

ALLOC_PER=$(awk "BEGIN{printf \"%.2f\", ($A2-$A1)/$DELTA}")
BYTES_PER=$(awk "BEGIN{printf \"%.1f\", ($B2-$B1)/$DELTA}")
FREES_PER=$(awk "BEGIN{printf \"%.2f\", ($F2-$F1)/$DELTA}")
KB_PER=$(awk "BEGIN{printf \"%.1f\", ($B2-$B1)/$DELTA/1024}")

{
  echo "# alloc wynik: $LABEL"
  echo
  echo "- data: $(date -Iseconds)"
  echo "- binarka: $BIN (RDB_ALLOC_COUNTER)"
  echo "- workload: rec205-detect.rql, metoda roznicy N1=$N1 N2=$N2 (delta=$DELTA), $PIN"
  echo
  echo "| wskaznik (na interwal) | wartosc |"
  echo "|---|---|"
  echo "| **alokacje / interwal** | **$ALLOC_PER** |"
  echo "| bajty / interwal | $BYTES_PER (${KB_PER} KiB) |"
  echo "| zwolnienia / interwal | $FREES_PER |"
  echo
  echo "surowe totale: N1 allocs=$A1 bytes=$B1 frees=$F1 ; N2 allocs=$A2 bytes=$B2 frees=$F2"
} | tee "$OUTDIR/$LABEL-alloc.md"

echo "== zapisano: $OUTDIR/$LABEL-alloc.md =="
