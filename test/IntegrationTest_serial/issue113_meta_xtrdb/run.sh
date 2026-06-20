#!/bin/bash
# Weryfikacja sidecar .meta dla str_null przez xtrdb (issue #113).
# Logika wyniesiona ze skryptu (zamiast bash -c w add_test): pozwala użyć
# ${TMPDIR:-/tmp} dla ścieżki blokady serwera (xretractor tworzy lock w
# temp_directory_path(), zob. lockManager.cpp). Bez tego pętle synchronizacji
# wieszają się gdy TMPDIR != /tmp.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
xqry -s str_null -k -m 3 > /dev/null
while [ -f "$LOCK" ]; do sleep 0.1; done
test -f temp/str_null.meta
test "$(stat -c%s temp/str_null.meta)" -gt 16
xtrdb noprompt < term.script > out.txt
grep -q 'meta: temp/str_null.meta' out.txt
! grep -q 'meta file not found' out.txt
