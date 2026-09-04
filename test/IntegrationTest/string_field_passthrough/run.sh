#!/bin/bash
# Pole STRING ze zrodla tekstowego ma dotrzec do wyrazenia w SELECT, a jego typ do
# deskryptora artefaktu (pozycja 12 w usecases/requested.md).
#
# Zrodlo jest CELOWO bez cudzyslowow — to ten wariant byl zepsuty. Przeczytany token
# byl wyrzucany, a skan cudzyslowu konsumowal reszte pliku, wiec dla wiersza `42 7`
# pole txt wychodzilo puste, k dostawalo 42, a deskryptor mial dwa pola INTEGER.
#
# `Length(txt)` przypina druga polowe tej samej sciezki: wartosc pola STRING nie tylko
# dochodzi do wyrazenia, ale daje sie w nim policzyc. Zaden test jednostkowy tego nie
# pokrywa — tam argument jest literalem, a nie odczytem z payloadu. Wiersz `betatest`
# jest w danych po to, zeby trafic w przypadek graniczny: wartosc wypelnia STRING[8]
# co do bajtu, wiec nie ma bajtu zerowego, na ktorym getItemVT moglby przyciac.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 4 > out.txt
server_wait_exit
grep -F '42 7 2' out.txt
grep -F '43 8 2' out.txt
grep -F '44 9 2' out.txt
grep -F 'betatest 10 8' out.txt
grep -F 'STRING dst_0[8]' temp/dst.desc
grep -F 'INTEGER dst_1' temp/dst.desc
grep -F 'INTEGER dst_2' temp/dst.desc
