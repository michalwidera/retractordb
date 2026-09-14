#!/bin/bash
# KSZTALT: R3 nie reasocjuje stalych nad slotem FLOAT rekordu wejsciowego okna i reduktora.
#
# Wartosci liczy value.sh (regula przy issue202_hash_shift_e2e w CLAUDE.md).
set -e
mkdir -p temp

xretractor query.rql -c > plan.txt

# Program pola od naglowka `<pole>:` do nastepnego naglowka pola lub strumienia, tokeny w jednej linii.
program() { awk -v f="$1:" '$1 == f { on = 1; next } on && ($1 ~ /^[a-z0-9_]+:$/ || /^[^[:space:]]/) { exit } on { print $1 }' plan.txt | tr '\n' ' ' | sed 's/ $//'; }

# Do 2026-09-14 `w_0`, `r_0` i `r_1` wychodzily `PUSH_ID(...) PUSH_VAL(3) ADD`.
check() {
  local actual
  actual="$(program "$1")"
  test "$actual" = "$2" || {
    echo "REGRESJA: program pola '$1' przepisany nad slotem FLOAT"
    echo "oczekiwane: $2"
    echo "jest:       $actual"
    cat plan.txt
    exit 1
  }
}
check w_0 "PUSH_ID(w[1]) PUSH_VAL(1) ADD PUSH_VAL(2) ADD"
check w_1 "PUSH_ID(w[1]) PUSH_VAL(1) ADD PUSH_VAL(2) ADD"
check r_0 "PUSH_ID(r[0]) PUSH_VAL(1) ADD PUSH_VAL(2) ADD"
check r_1 "PUSH_ID(r[0]) PUSH_VAL(1) ADD PUSH_VAL(2) ADD"
