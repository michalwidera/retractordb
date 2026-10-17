#!/bin/bash
# Pozycja 6 z paper-arXiv/debs/done/requested.md: nie bylo trybu "licz do konca wejscia". Jednostka -m
# to liczba pobudek planisty, wiec wartosc trzeba bylo dobierac probowaniem dla kazdej serii
# danych osobno, a po wyczerpaniu wejscia zrodlo tekstowe zawijalo sie na poczatek pliku
# i po cichu produkowalo rekordy z danych, ktore juz raz przeszly.
#
# Test dowodzi trzech rzeczy:
#  1. --until-eof bez zadnego -m daje DOKLADNIE ten sam artefakt, co recznie dobrane -m,
#  2. --until-eof nie zawija zrodla, choc plan nie niesie ONESHOT - a bez tej flagi zawija
#     (kontrola negatywna, bez ktorej punkt 1 przeszedlby takze przy zignorowanym trybie),
#  3. przy wielu zrodlach przebieg konczy sie na PIERWSZYM wyczerpanym wejsciu.
set -e
. "$(dirname "$0")/../portable.sh"

rm -rf temp oracle two
mkdir -p temp oracle two

# 1. Przebieg wzorcowy: ONESHOT + trafione -m 9 (8 rekordow wejscia, -m 8 daje 7,
#    -m 10 doklada rekord all-null). Przebieg badany: bez ONESHOT, bez -m, tylko -u.
xretractor oracle.rql -k -r -f -m 9
xretractor query.rql -k -r -f -u

cmp oracle/dst temp/dst

# 2. Kontrola negatywna: ten sam plan bez -u, z zapasem pobudek, zawija zrodlo i wychodzi
#    poza rozmiar wejscia. Gdyby -u byl ignorowany, artefakt z punktu 1 mialby ten rozmiar.
size_untileof=$(file_size temp/dst)
rm -rf temp && mkdir temp
xretractor query.rql -k -r -f -m 14
size_wrapped=$(file_size temp/dst)

echo "until-eof=${size_untileof} B  wrapped=${size_wrapped} B"

if [ "$size_wrapped" -le "$size_untileof" ]; then
  echo "FAIL: przebieg bez -u mial sie zawinac i przekroczyc rozmiar wejscia"
  exit 1
fi

# 3. Dwa zrodla, 8 i 3 rekordy: stop na pierwszym wyczerpanym, wiec oba wyjscia po 3 rekordy.
LOG="${TMPDIR:-/tmp}/xretractor.log"
log_mark=$(wc -l < "$LOG" 2>/dev/null || echo 0)
xretractor two_sources.rql -k -r -f -u
d1=$(file_size two/d1)
d2=$(file_size two/d2)

echo "d1=${d1} B  d2=${d2} B"

if [ "$d1" != "$d2" ]; then
  echo "FAIL: przebieg mial sie zatrzymac na krotszym zrodle (d1=${d1} B, d2=${d2} B)"
  exit 1
fi

# 4. Log nazywa wyczerpane zrodlo (#521, etap 9b): krotszym wejsciem jest s2 (short.txt). Wpis ma
#    poziom INFO, ktory Release wycina z kompilacji - stad RDB_INFO_ACTIVE z CMakeLists.txt.
if [ "${RDB_INFO_ACTIVE:-1}" = "1" ]; then
  if ! tail -n +"$((log_mark + 1))" "$LOG" | grep -qF "End of input on declared stream 's2' - stopping (--until-eof)."; then
    echo "FAIL: log nie nazwal wyczerpanego zrodla s2; dopisane linie:"
    tail -n +"$((log_mark + 1))" "$LOG"
    exit 1
  fi
fi
