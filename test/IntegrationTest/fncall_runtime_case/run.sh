#!/bin/bash
# Funkcje gramatyki pisane wielką literą muszą się WYKONAĆ, nie tylko skompilować.
#
# Pułapka, którą ten test zamyka: `xretractor query.rql -c` przechodził, bo tryb
# compile-only nie woła ewaluatora, a wykonanie wywracało się na
# "Unsupported function call: Sqrt". Test kompilujący (Pattern4/query-crc.rql)
# tej różnicy nie widzi, więc ten test musi plan URUCHOMIĆ i odczytać wartości.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 2 > out.txt
server_wait_exit
# Sqrt(16)=4, Ceil(2.5)=3, Floor(2.5)=2
grep -F '4 3 2' out.txt
# Sqrt(81)=9, Ceil(-1.5)=-1, Floor(-1.5)=-2
grep -F '9 -1 -2' out.txt
