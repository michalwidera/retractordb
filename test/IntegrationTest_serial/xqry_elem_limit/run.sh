#!/bin/bash
# xqry -m N ogranicza liczbę elementów do dokładnie N (issue elimitqry).
# Logika wyniesiona ze skryptu (zamiast bash -c w add_test): pozwala użyć
# ${TMPDIR:-/tmp} dla ścieżki blokady serwera (xretractor tworzy lock w
# temp_directory_path(), zob. lockManager.cpp). Bez tego pętle synchronizacji
# wieszają się gdy TMPDIR != /tmp.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
xqry -s dst -k -m 5 > out.txt
while [ -f "$LOCK" ]; do sleep 0.1; done
[ "$(wc -l < out.txt)" -eq 5 ]
