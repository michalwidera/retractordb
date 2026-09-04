#!/bin/bash
# null z pliku źródłowego przesyłany przez IPC do xqry (issue #113).
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s str_null -k -m 3 > out.txt
server_wait_exit
grep -F '10 null' out.txt
grep -F '20 30' out.txt
