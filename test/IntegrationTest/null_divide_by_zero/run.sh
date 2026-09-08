#!/bin/bash
# Dzielenie przez zero -> NULL, a strumien leci dalej (nie przerywa sie).
# Dane: 4, 0, 5  ->  100/4 = 25, 100/0 = null, 100/5 = 20.
#
# Rekord PO dzieleniu przez zero jest tu istota testu: gdyby brak wyniku byl
# obslugiwany wyjatkiem, trzeci rekord nigdy by nie powstal.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 3 > out.txt
server_wait_exit
grep -F '25' out.txt
grep -F 'null' out.txt
grep -F '20' out.txt
