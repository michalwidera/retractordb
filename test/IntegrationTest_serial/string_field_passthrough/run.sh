#!/bin/bash
# Pole STRING ze zrodla tekstowego ma dotrzec do wyrazenia w SELECT, a jego typ do
# deskryptora artefaktu (pozycja 12 w usecases/requested.md).
#
# Zrodlo jest CELOWO bez cudzyslowow — to ten wariant byl zepsuty. Przeczytany token
# byl wyrzucany, a skan cudzyslowu konsumowal reszte pliku, wiec dla wiersza `42 7`
# pole txt wychodzilo puste, k dostawalo 42, a deskryptor mial dwa pola INTEGER.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 3 > out.txt
server_wait_exit
grep -F '42 7' out.txt
grep -F '43 8' out.txt
grep -F '44 9' out.txt
grep -F 'STRING dst_0[8]' temp/dst.desc
grep -F 'INTEGER dst_1' temp/dst.desc
