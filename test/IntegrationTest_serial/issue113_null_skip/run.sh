#!/bin/bash
# Weryfikacja flagi -n (skip-null) w xqry (issue #113). Arg $1: skip|noskip.
set -e
. "$(dirname "$0")/../serverlib.sh"
mode="$1"
server_start query.rql -k -x
if [ "$mode" = "skip" ]; then
  xqry -s str_null -n -k -m 4 > out_skip.txt
  OUT=out_skip.txt
else
  xqry -s str_null -k -m 4 > out_noskip.txt
  OUT=out_noskip.txt
fi
server_wait_exit
grep -F '10 null' "$OUT"
grep -F '20 30' "$OUT"
if [ "$mode" = "skip" ]; then
  ! grep -qF 'null null' "$OUT"
else
  grep -qF 'null null' "$OUT"
fi
