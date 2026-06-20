#!/bin/bash
# Konwersja numeric->STRING przez to_string() (issue #128).
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
grep -F '42_test' out.txt
grep -F '3.140000' out.txt
grep -F '100_test' out.txt
grep -F -- '-1.500000' out.txt
grep -F '7_test' out.txt
grep -F '0.000000' out.txt
grep -F 'STRING dst_0[21]' temp/dst.desc
grep -F 'STRING dst_1[16]' temp/dst.desc
