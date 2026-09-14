#!/bin/bash
# KSZTALT: typy pol wyjsciowych w `-c`. Jawna lista i `SELECT *` nad tym samym wezlem daja ten sam typ.
#
# Wartosci liczy value.sh (regula przy issue202_hash_shift_e2e w CLAUDE.md).
set -e
mkdir -p temp

xretractor query.rql -c > plan.txt

# Typy pol strumienia: linie `<TAB>pole: TYP` od naglowka `strumien(` do nastepnego naglowka.
types() { awk -v s="$1(" 'index($0, s) == 1 { on = 1; next } /^[^[:space:]]/ { on = 0 } on && /^\t[A-Za-z0-9_]+: / { print $2 }' plan.txt | tr '\n' ' ' | sed 's/ $//'; }

check() {
  local actual
  actual="$(types "$1")"
  test "$actual" = "$2" || {
    echo "REGRESJA: typy pol strumienia '$1'"
    echo "oczekiwane: $2"
    echo "jest:       $actual"
    cat plan.txt
    exit 1
  }
}

# Do 2026-09-14: w, av, mx wychodzily INTEGER (obciecie), sb i st RATIONAL (przepelnienie i 1/3),
# acc_1 i acc_2 INTEGER.
check w "FLOAT FLOAT"
check ws "FLOAT FLOAT"
check av "RATIONAL RATIONAL RATIONAL"
check avs "RATIONAL"
check mx "FLOAT FLOAT"
check sb "DOUBLE"
check st "DOUBLE"
check acc "FLOAT FLOAT FLOAT"
