#!/bin/bash
# Ad-hoc DECLARE przy zachowanym .desc o innym REF (#278).
#
# Magazyn deklaracji bierze REF z wczytanego .desc, a Descriptor::operator== porownuje tylko
# sloty danych. Start i przeladowanie planu odmawiaja rozjazdu REF w checkDescriptorFiles(),
# kanal ad-hoc tego nie robil: REF z zachowanego pliku wygrywal z planem, a przy DISPOSABLE
# koniec instancji kasowal plik, ktorego plan nie wskazywal. Od #278 ad-hoc woluje te sama
# kontrole dla nowo dodawanych strumieni - przed importem, kompilacja i roszczeniem nazw.
#
# Test sprawdza odmowe z powodem w formie kontroli wstepnej (rdb::storage odmowilby tez, ale
# innym komunikatem i dopiero po imporcie), zycie serwera, nietkniety plik spod REF, brak nazwy
# w planie i na magistrali oraz - wada lustrzana - to, ze to samo zapytanie bez zachowanego
# .desc przechodzi. Bez ostatniej kontroli naprawa "odmawiaj zawsze" tez bylaby zielona.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
rm -f temp/late.desc planned.txt foreign.txt
printf '1\n2\n3\n' > planned.txt
printf '7\n' > foreign.txt
cp foreign.txt foreign.orig

server_start query.rql -k

printf '{\tINTEGER a\n\tREF "foreign.txt"\n\tTYPE TEXTSOURCE\n}\n' > temp/late.desc
cp temp/late.desc late.desc.orig

rc=0
xqry -a "DECLARE a INTEGER STREAM late, 1/8 TEXTFILE 'planned.txt' DISPOSABLE" > out_fail.txt 2> err_fail.txt || rc=$?
expected="Rejected: stream 'late': temp/late.desc was written for source 'foreign.txt' and the plan reads 'planned.txt'; remove temp/late.desc to start the stream afresh"
if [ "$rc" -eq 0 ] || ! grep -qF "$expected" err_fail.txt; then
  echo "ad-hoc z niezgodnym REF nie zwrocil odmowy kontroli wstepnej (kod $rc)"
  cat out_fail.txt err_fail.txt
  exit 1
fi
if ! kill -0 "$_server_pid" 2> /dev/null; then
  echo "serwer zginal po odmowie ad-hoc"
  exit 1
fi
if ! cmp -s foreign.txt foreign.orig || ! cmp -s temp/late.desc late.desc.orig; then
  echo "odmowa zmienila plik spod REF albo zachowany .desc"
  exit 1
fi
xqry -d > dir_after_fail.txt
bus_own > bus_after_fail.txt
if grep -qw late dir_after_fail.txt || grep -qE '\|[[:space:]]+late$' bus_after_fail.txt; then
  echo "odrzucony strumien 'late' zostal w planie lub na magistrali:"
  cat dir_after_fail.txt bus_after_fail.txt
  exit 1
fi

# Wada lustrzana: bez zachowanego .desc to samo zapytanie przechodzi.
rm -f temp/late.desc
rc=0
xqry -a "DECLARE a INTEGER STREAM late, 1/8 TEXTFILE 'planned.txt'" > out_ok.txt 2> err_ok.txt || rc=$?
xqry -d > dir_after_ok.txt
if [ "$rc" -ne 0 ] || ! grep -qw late dir_after_ok.txt; then
  echo "ad-hoc bez zachowanego .desc odrzucony (kod $rc) albo 'late' nie trafil do planu"
  cat out_ok.txt err_ok.txt dir_after_ok.txt
  exit 1
fi
