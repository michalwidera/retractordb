#!/bin/bash
# KSZTALT: pole odwolujace sie do wlasnej nazwy dostaje typ z rekordu FROM, nie z wlasnego wyjscia.
#
# Wartosci liczy value.sh (regula przy issue202_hash_shift_e2e w CLAUDE.md).
set -e
mkdir -p temp

xretractor query.rql -c > plan.txt

# Do 2026-09-14 `merged_0` wychodzilo INTEGER (wlasne pole 0), a `merged_1` BYTE (wlasne pole 2).
expected="merged_0: BYTE merged_1: INTEGER merged_2: BYTE merged_3: INTEGER"
actual="$(grep -o 'merged_[0-9]: [A-Z]*' plan.txt | tr '\n' ' ' | sed 's/ $//')"
test "$actual" = "$expected" || {
  echo "REGRESJA: zle typy pol 'merged'"
  echo "oczekiwane: $expected"
  echo "jest:       $actual"
  cat plan.txt
  exit 1
}
