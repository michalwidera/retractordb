#!/bin/bash
# covers: rqlparser.parser-rql-string.17~1, streaminstance.stream-instance.11~1, datamodel.data-model.17~1
#
# Akcja DO SET (#531, etap B). Kazdy plan liczony offline (`-f -u`) DWA razy, a wynik sprawdzany:
#
#   1. wobec wyroczni niezaleznej oracle.py, liczacej stan sekwencyjnie od startu przebiegu (R3) -
#      wartosci, typy pol z `.desc` i obecnosc zrzutow DUMP;
#   2. drugi przebieg wobec pierwszego, bajtowo (compare.sh): stan zmiennych nie moze wprowadzic
#      niedeterminizmu.
#
# Plan slot.rql sprawdza D1 dla czytelnika przed regula i za nia w planie; ze ten uklad naprawde
# zachodzi, pokazuje zrzut `-c` (kolejnosc wezlow), a nie nazwy strumieni.
#
# Test sprawdza wylacznie WARTOSCI, ktore nie zaleza od RDB_OPT_*, wiec nie dostaje DISABLED pod
# ablacja.
set -e
. "$(dirname "$0")/../portable.sh"
export LC_ALL=C

order=$(xretractor slot.rql -c | grep -oE '^(early|writer|late)\(' | tr -d '(' | tr '\n' ' ')
if [ "$order" != "early writer late " ]; then
  echo "slot.rql: kolejnosc wezlow '$order', oczekiwana 'early writer late' - test D1 nie widzi czytelnika przed i za regula"
  exit 1
fi

# Wypis jednego przebiegu: rekordy kazdego strumienia SELECT, typy z `.desc` i lista zrzutow.
listing() {
  local rql="$1"
  for stream in $(sed -n 's/^SELECT .* STREAM \([A-Za-z_0-9]*\) FROM .*/\1/p' "$rql"); do
    echo "== $stream"
    (cd temp && printf 'open %s\nlist 1000\nquit\n' "$stream" | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
    echo "== desc $stream"
    cat "temp/$stream.desc"
    echo
  done
  echo "== dumps"
  (cd temp && find . -name '*_dump*' | sort)
}

status=0
for rql in *.rql; do
  plan=${rql%.rql}
  for run in 1 2; do
    rm -rf temp
    mkdir temp
    xretractor "$rql" -k -f -u > /dev/null
    listing "$rql" > "$plan.run$run.out"
  done
  bash ../compare.sh "$plan.run1.out" "$plan.run2.out" || status=1
  python3 oracle.py "$rql" "$plan.run1.out" || status=1
done
exit $status
