#!/bin/bash
# Konwersja STRING->numeric przez to_integer/to_float/to_double (issue #128).
# Logika wyniesiona ze skryptu (zamiast bash -c w add_test): pozwala użyć
# ${TMPDIR:-/tmp} dla ścieżki blokady serwera (xretractor tworzy lock w
# temp_directory_path(), zob. lockManager.cpp). Bez tego pętle synchronizacji
# wieszają się gdy TMPDIR != /tmp.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
xqry -s dst -k -m 4 > out.txt
while [ -f "$LOCK" ]; do sleep 0.1; done
grep -F '11 10 10' out.txt
grep -F '26 25 25' out.txt
grep -F -- '-4 -5 -5' out.txt
grep -F 'null null null' out.txt
