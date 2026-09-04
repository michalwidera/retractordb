#!/bin/bash
# Wewnetrzny format sidecara .meta po przebiegu z nullami (issue #113):
# naglowek, rozmiar wpisu, interwal probkowania i bitset per rekord.
#
# Logika wyniesiona z `bash -c` w add_test — powod jak w operations/run.sh.
# Wczesniejsza wersja poprzedzala start czekaniem na zwolnienie blokady przez
# obca instancje; to zadanie przejela bramka higieny w ../serverlib.sh, ktora
# obarcza winowajce zamiast nastepnej ofiary.
set -e
. "$(dirname "$0")/../serverlib.sh"

mkdir -p temp
rm -f temp/str_null temp/str_null.* temp/core0 temp/core0.*
server_start query.rql -k -x
xqry -s str_null -k -m 5 > /dev/null
server_wait_exit

xtrdb noprompt < term.script > out.txt
grep -Fq 'header size: 8' out.txt
grep -Fq 'entry size: 18' out.txt
grep -Fq 'sampling interval: 1/1' out.txt
grep -Fq 'entry[0] count:1 gap:0 bitsetSize:2 bitsetHex:03' out.txt
grep -Fq 'entry[1] count:2 gap:0 bitsetSize:2 bitsetHex:02' out.txt
grep -Fq 'entry[2] count:1 gap:0 bitsetSize:2 bitsetHex:00' out.txt
grep -Fq 'entries: 3' out.txt
