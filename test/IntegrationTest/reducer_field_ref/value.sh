#!/bin/bash
# WARTOSC: postac zmaterializowana WYKONUJE sie i daje wlasciwe liczby.
#
# Samo `-c` nie wystarcza — caly defekt polegal na tym, ze `-c` przechodzilo, a plan ginal
# w wykonaniu przy zerze rekordow. Dlatego ten wpis uruchamia silnik i czyta artefakt.
#
# Przebieg jest OFFLINE (`--until-eof --no-clock`): serwer czekajacy na klienta liczy sloty
# do chwili zatrzymania, wiec liczba rekordow zalezalaby od zegara. Dwa wiersze `data.txt`
# to dokladnie dwa rekordy. Wartosci czytamy z ARTEFAKTU, a nie przez klienta: klient
# subskrybuje sie juz w trakcie i jego pierwsze sloty zaleza od wyscigu ze startem planu.
set -e
mkdir -p temp
rm -f temp/m temp/m.desc temp/m.meta temp/mul temp/mul.desc temp/mul.meta

xretractor query.rql -k -u -f

# --- 1. Typ i liczba rekordow --------------------------------------------------------------
# Reduktor daje JEDNO pole RATIONAL (reductionResultField), niezaleznie od typu zrodla.
xtrdb -n -s temp/m > map_m.txt
grep -E 'RATIONAL +avg' map_m.txt
grep -E 'Records: 2' map_m.txt

xtrdb -n -s temp/mul > map_mul.txt
grep -E 'RATIONAL +mul_0' map_mul.txt
grep -E 'Records: 2' map_mul.txt

# --- 2. Wartosci ---------------------------------------------------------------------------
# RATIONAL to para 4-bajtowych licznika i mianownika (ut_rdb pinuje ten uklad), wiec `od`
# pokazuje kolejno licznik i mianownik kazdego rekordu.
#
# AVG redukuje WSZYSTKIE sloty biezacego rekordu: (3+5)/2 = 4, (7+11)/2 = 9.
test "$(od -An -tu4 temp/m | tr -s ' ' | sed 's/^ //;s/ $//')" = "4 1 9 1" || {
  echo "REGRESJA: zle wartosci reduktora"; od -An -tu4 temp/m; exit 1; }

# Pole liczone NAD wynikiem reduktora: 4*2 = 8, 9*2 = 18.
test "$(od -An -tu4 temp/mul | tr -s ' ' | sed 's/^ //;s/ $//')" = "8 1 18 1" || {
  echo "REGRESJA: zle wartosci nad reduktorem"; od -An -tu4 temp/mul; exit 1; }
