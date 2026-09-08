#!/bin/bash
# isnull(field): 1 gdy null, 0 gdy nie (issue #121).
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 3 > out.txt
server_wait_exit
grep -F '0 1' out.txt
grep -F '1 0' out.txt
grep -F '0 0' out.txt
