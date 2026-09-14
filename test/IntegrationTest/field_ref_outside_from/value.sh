#!/bin/bash
# WARTOSC: postac poprawna (query.rql) czyta `core0.b` i `core1.c` z rekordu `merged`.
#
# Przebieg OFFLINE (`-f`) z budzetem 12 slotow: liczba rekordow nie zalezy od zegara.
# Wartosci czytamy z ARTEFAKTU, nie przez klienta.
set -e
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -r -m 12

# `result` = dwa INTEGER-y. `core0` (co 0,1) i `core1` (co 0,2) zawijaja swoje pliki, `core1`
# trzyma wartosc przez dwa sloty `merged`. Drugie pole niesie 100..400 — do 2026-09-14
# odwolanie `core1[0]` przy `FROM merged` dawalo tu po cichu `core0.a`.
expected="10 100 20 100 30 200 40 200 50 300 60 300 10 400 20 400 30 100 40 100"
actual="$(od -An -tu4 -v temp/result | tr -s ' \n' ' ' | sed 's/^ //;s/ $//')"
test "$actual" = "$expected" || {
  echo "REGRESJA: zle wartosci 'result'"
  echo "oczekiwane: $expected"
  echo "jest:       $actual"
  exit 1
}
