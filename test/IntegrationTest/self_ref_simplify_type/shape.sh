#!/bin/bash
# KSZTALT: R3 nie reasocjuje stalych nad polem FLOAT, do ktorego odwolanie prowadzi wlasna nazwa.
#
# Wartosci liczy value.sh (regula przy issue202_hash_shift_e2e w CLAUDE.md).
set -e
mkdir -p temp

xretractor query.rql -c > plan.txt

# Program pola od naglowka `merged_N:` do nastepnego naglowka, tokeny w jednej linii.
program() { awk -v f="$1:" '$1 == f { on = 1; next } on && /^[[:space:]]*merged_[0-9]+:/ { exit } on { print $1 }' plan.txt | tr '\n' ' ' | sed 's/ $//'; }

# Do 2026-09-14 `merged_0` wychodzilo `PUSH_ID(merged[3]) PUSH_VAL(3) ADD`.
expected="PUSH_ID(merged[3]) PUSH_VAL(1) ADD PUSH_VAL(2) ADD"
for field in merged_0 merged_1; do
  actual="$(program $field)"
  test "$actual" = "$expected" || {
    echo "REGRESJA: program pola '$field' przepisany nad polem FLOAT"
    echo "oczekiwane: $expected"
    echo "jest:       $actual"
    cat plan.txt
    exit 1
  }
done
