#!/bin/bash
# Weryfikacja sidecar .meta dla str_null przez xtrdb (issue #113).
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s str_null -k -m 3 > /dev/null
server_wait_exit
test -f temp/str_null.meta
test "$(stat -c%s temp/str_null.meta)" -gt 16
xtrdb noprompt < term.script > out.txt
grep -q 'meta: temp/str_null.meta' out.txt
! grep -q 'meta file not found' out.txt
