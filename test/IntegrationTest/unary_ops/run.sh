#!/bin/bash
# Wartosci operatorow jednoargumentowych liczone offline jednym przejsciem przez dane (#328).
# Wypis: rekordy strumienia `uno` przez `xtrdb list`, typy pol z `.desc` i lista plikow zrzutu
# reguly `pos`, ktora ma odpalic dokladnie raz - na wierszu z ujemnym INTEGER.
set -e
. "$(dirname "$0")/../portable.sh"
export LC_ALL=C

rm -rf temp
mkdir temp
# `-f -u`: offline do wyczerpania zrodla - 4 wiersze danych to 4 rekordy.
xretractor unary.rql -k -f -u > /dev/null
{
  echo "== uno"
  (cd temp && printf 'open uno\nlist 1000\nquit\n' | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
  echo "== desc uno"
  cat temp/uno.desc
  echo  # .desc nie konczy sie znakiem nowej linii
  echo "== dumps"
  (cd temp && find . -name '*_dump*' | sort)
} > unary.out
bash ../compare.sh --ignore-eol unary.pattern unary.out
