#!/bin/bash
# covers: rqlparser.parser-rql-string.20~1, rqlparser.parser-rql-string.21~1
#
# Jawny typ stalej LET (#547) na sciezce wykonania: stala DOUBLE, RATIONAL i UINT jest w wyrazeniu
# tokenem PUSH_VAL tego typu, czego bez adnotacji nie da sie zapisac. typed.rql liczony offline
# (`-f -u`), wypis `xtrdb` (wartosci i typy pol z `.desc`) wobec wyroczni oracle.py.
#
# Test sprawdza wylacznie WARTOSCI i TYPY, ktore nie zaleza od RDB_OPT_*, wiec nie dostaje DISABLED
# pod ablacja.
set -e
. "$(dirname "$0")/../serverlib.sh"
export LC_ALL=C

rm -rf temp
mkdir temp
xretractor typed.rql -k -f -u > /dev/null
{
  echo "== wynik"
  (cd temp && printf 'open wynik\nlist 1000\nquit\n' | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
  echo "== desc wynik"
  cat temp/wynik.desc
} > typed.out
python3 oracle.py typed.out
