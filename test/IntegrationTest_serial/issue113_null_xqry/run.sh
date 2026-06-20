#!/bin/bash
# null z pliku źródłowego przesyłany przez IPC do xqry (issue #113).
# Logika wyniesiona ze skryptu (zamiast bash -c w add_test): pozwala użyć
# ${TMPDIR:-/tmp} dla ścieżki blokady serwera (xretractor tworzy lock w
# temp_directory_path(), zob. lockManager.cpp). Bez tego pętle synchronizacji
# wieszają się gdy TMPDIR != /tmp.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
xqry -s str_null -k -m 3 > out.txt
while [ -f "$LOCK" ]; do sleep 0.1; done
grep -F '10 null' out.txt
grep -F '20 30' out.txt
