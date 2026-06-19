#!/bin/bash
# Weryfikacja flagi -n (skip-null) w xqry (issue #113). Arg $1: skip|noskip.
# Logika wyniesiona ze skryptu (zamiast bash -c w add_test): pozwala użyć
# ${TMPDIR:-/tmp} dla ścieżki blokady serwera (xretractor tworzy lock w
# temp_directory_path(), zob. lockManager.cpp). Bez tego pętle synchronizacji
# wieszają się gdy TMPDIR != /tmp.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
mode="$1"
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
if [ "$mode" = "skip" ]; then
  xqry -s str_null -n -k -m 4 > out_skip.txt
  OUT=out_skip.txt
else
  xqry -s str_null -k -m 4 > out_noskip.txt
  OUT=out_noskip.txt
fi
while [ -f "$LOCK" ]; do sleep 0.1; done
grep -F '10 null' "$OUT"
grep -F '20 30' "$OUT"
if [ "$mode" = "skip" ]; then
  ! grep -qF 'null null' "$OUT"
else
  grep -qF 'null null' "$OUT"
fi
