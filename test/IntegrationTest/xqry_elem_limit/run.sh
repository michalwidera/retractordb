#!/bin/bash
# xqry -m N ogranicza liczbę elementów do dokładnie N (issue elimitqry).
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 5 > out.txt
server_wait_exit
[ "$(wc -l < out.txt)" -eq 5 ]
