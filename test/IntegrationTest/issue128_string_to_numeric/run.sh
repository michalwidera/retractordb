#!/bin/bash
# Konwersja STRING->numeric przez to_integer/to_float/to_double (issue #128).
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 4 > out.txt
server_wait_exit
grep -F '11 10 10' out.txt
grep -F '26 25 25' out.txt
grep -F -- '-4 -5 -5' out.txt
grep -F 'null null null' out.txt
