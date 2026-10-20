#!/bin/bash
# covers: rqlparser.parser-rql-string.18~1, rqlparser.parser-rql-string.19~1
#
# Jawny typ zmiennej VAR (#546, etap H) na sciezce wykonania - typy UINT i DOUBLE zmienna dostala
# dopiero z adnotacja, wiec odczyt PUSH_VAR, zatwierdzenie zapisu po slocie (D1) i `xqry --vars`
# widza je tu po raz pierwszy.
#
#   1. typed.rql liczony offline (`-f -u`) DWA razy: wynik wobec wyroczni oracle.py (wartosci i typy
#      pol z `.desc`) oraz drugi przebieg wobec pierwszego, bajtowo (compare.sh);
#   2. serwer z --xqrywait: `xqry --vars` przed pierwszym slotem pokazuje typy z adnotacji i wartosci
#      z deklaracji, w tym RATIONAL 1/4 z literalu 0.25.
#
# Test sprawdza wylacznie WARTOSCI i TYPY, ktore nie zaleza od RDB_OPT_*, wiec nie dostaje DISABLED
# pod ablacja.
set -e
. "$(dirname "$0")/../serverlib.sh"
export LC_ALL=C

# Wypis jednego przebiegu: rekordy strumienia `stan` i jego typy z `.desc`.
listing() {
  echo "== stan"
  (cd temp && printf 'open stan\nlist 1000\nquit\n' | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
  echo "== desc stan"
  cat temp/stan.desc
}

for run in 1 2; do
  rm -rf temp
  mkdir temp
  xretractor typed.rql -k -f -u > /dev/null
  listing > "typed.run$run.out"
done
bash ../compare.sh typed.run1.out typed.run2.out
python3 oracle.py typed.run1.out

rm -rf temp
mkdir temp
server_start typed.rql -k -r -x
xqry --vars | tr -d '\r' > declared.txt
cat > expected.txt <<'TABLE'
name    | type     | value
--------+----------+------
licznik | UINT     | 0
suma    | DOUBLE   | 0
prog    | RATIONAL | 1/4
TABLE
diff -u expected.txt declared.txt || {
  echo "xqry --vars przed pierwszym slotem nie pokazal typow z adnotacji i wartosci z deklaracji"
  exit 1
}
xqry -k
server_wait_exit
