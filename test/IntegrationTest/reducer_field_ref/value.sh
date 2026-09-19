#!/bin/bash
# WARTOSC: postac zmaterializowana WYKONUJE sie i daje wlasciwe liczby.
#
# Samo `-c` nie wystarcza - caly defekt polegal na tym, ze `-c` przechodzilo, a plan ginal
# w wykonaniu przy zerze rekordow. Dlatego ten wpis uruchamia silnik i czyta artefakt.
#
# Przebieg jest OFFLINE (`--until-eof --no-clock`): serwer czekajacy na klienta liczy sloty
# do chwili zatrzymania, wiec liczba rekordow zalezalaby od zegara. Dwa wiersze `data.txt`
# to dokladnie dwa rekordy. Wartosci czytamy z ARTEFAKTU, a nie przez klienta: klient
# subskrybuje sie juz w trakcie i jego pierwsze sloty zaleza od wyscigu ze startem planu.
set -e
. "$(dirname "$0")/../portable.sh"
mkdir -p temp
rm -f temp/m temp/m.desc temp/m.meta temp/mul temp/mul.desc temp/mul.meta

xretractor query.rql -k -u -f

# --- 1. Typ i liczba rekordow --------------------------------------------------------------
# Reduktor daje JEDNO pole typu z reductionResultField(): nad zrodlem DOUBLE - DOUBLE. Do 2026-09-14
# stal tu RATIONAL, wpisywany przez buildOutputSchema() na sztywno niezaleznie od typu zrodla.
xtrdb -n -s temp/m > map_m.txt
grep -E 'DOUBLE +avg' map_m.txt
grep -E 'Records: 2' map_m.txt

xtrdb -n -s temp/mul > map_mul.txt
grep -E 'DOUBLE +mul_0' map_mul.txt
grep -E 'Records: 2' map_mul.txt

# --- 2. Wartosci ---------------------------------------------------------------------------
# Nie przez `od`: jego format `f4`/`f8` obcina cyfry inaczej na GNU i na BSD (patrz
# read_binary_values w portable.sh).
doubles() { read_binary_values "$1" f8; }

# AVG redukuje WSZYSTKIE sloty biezacego rekordu: (3+5)/2 = 4, (7+11)/2 = 9.
test "$(doubles temp/m)" = "4 9" || {
  echo "REGRESJA: zle wartosci reduktora"; od -An -tf8 temp/m; exit 1; }

# Pole liczone NAD wynikiem reduktora: 4*2 = 8, 9*2 = 18.
test "$(doubles temp/mul)" = "8 18" || {
  echo "REGRESJA: zle wartosci nad reduktorem"; od -An -tf8 temp/mul; exit 1; }
