#!/bin/bash
# Dzielenie przez zero -> NULL, a strumien leci dalej (nie przerywa sie).
# Dane: 4, 0, 5  ->  100/4 = 25, 100/0 = null, 100/5 = 20.
#
# Rekord PO dzieleniu przez zero jest tu istota testu: gdyby brak wyniku byl
# obslugiwany wyjatkiem, trzeci rekord nigdy by nie powstal.
#
# ${TMPDIR:-/tmp} dla sciezki blokady serwera — zob. komentarz w issue121_isnull.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
xqry -s dst -k -m 3 > out.txt
while [ -f "$LOCK" ]; do sleep 0.1; done
grep -F '25' out.txt
grep -F 'null' out.txt
grep -F '20' out.txt
