#!/bin/bash
# Konwersja numeric->STRING przez to_string() (issue #128).
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 3 > out.txt
server_wait_exit
grep -F '42_test' out.txt
grep -F '3.140000' out.txt
grep -F '100_test' out.txt
grep -F -- '-1.500000' out.txt
grep -F '7_test' out.txt
grep -F '0.000000' out.txt
grep -F 'STRING dst_0[21]' temp/dst.desc
grep -F 'STRING dst_1[16]' temp/dst.desc
