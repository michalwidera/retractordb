#!/bin/bash
# Propagacja null przez SELECT do strumienia wynikowego (issue #121).
# Logika wyniesiona ze skryptu (zamiast bash -c w add_test): pozwala użyć
# ${TMPDIR:-/tmp} dla ścieżki blokady serwera (xretractor tworzy lock w
# temp_directory_path(), zob. lockManager.cpp). Bez tego pętle synchronizacji
# wieszają się gdy TMPDIR != /tmp.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
xqry -s dst -k -m 3 > out.txt
while [ -f "$LOCK" ]; do sleep 0.1; done
grep -F '10 null' out.txt
grep -F 'null 20' out.txt
grep -F '30 40' out.txt
