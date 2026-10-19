#!/usr/bin/env bash
# Stale LET znikaja przed wykonaniem (#440, D1/D4): parser podstawia liczby, wiec kompilator i silnik
# nie odrozniaja planu ze stalymi od planu z literalami.
#
#   1. Zrzuty `-c` (DOT z polami, programami, regulami i tagami; CSV; tryb gadatliwy) sa bajtowo
#      rowne - to glowny dowod - i zaden nie zawiera nazwy stalej.
#   2. Ten sam przebieg na tych samych danych daje bajtowo rowne pliki strumieni, deskryptory
#      i zrzuty reguly DUMP.
#
# Oba plany biegna pod ta sama nazwa pliku w osobnych katalogach, zeby sciezki w wyjsciu byly rowne.
set -e
here=$(pwd)
rm -rf run_literal run_let
for variant in literal let; do
  mkdir "run_$variant"
  cp "$variant.rql" "run_$variant/plan.rql"
  cp data.txt "run_$variant/"
done

compile_dumps() {
  # W trybie -c litera -f znaczy 'fields' w wyjsciu DOT.
  xretractor plan.rql -c -d -f -s -u -t >dump.dot 2>dump.err
  xretractor plan.rql -c -m >dump.csv 2>>dump.err
  xretractor plan.rql -c -v >dump.verbose 2>>dump.err
}
(cd run_literal && compile_dumps)
(cd run_let && compile_dumps)
for dump in dump.dot dump.csv dump.verbose; do
  [ -s "run_literal/$dump" ] || {
    echo "pusty zrzut run_literal/$dump"
    exit 1
  }
  bash ../compare.sh "run_literal/$dump" "run_let/$dump"
  if grep -q '?' "run_let/$dump"; then
    echo "zrzut run_let/$dump zawiera nazwe stalej:"
    grep '?' "run_let/$dump"
    exit 1
  fi
done
if grep -q 'constant' run_let/dump.err; then
  echo "plan ze stalymi dal ostrzezenie o stalych:"
  cat run_let/dump.err
  exit 1
fi

# Flaga -f (--no-clock) poza trybem -c zdejmuje czekanie na zegar scienny.
(cd run_literal && xretractor plan.rql -r -k -m 30 -f)
(cd run_let && xretractor plan.rql -r -k -m 30 -f)
cd run_literal
outputs=$(ls | grep -v -e '^plan.rql$' -e '^data.txt$' -e '^dump\.')
cd "$here"
[ -n "$outputs" ] || {
  echo "przebieg planu z literalami nie zostawil zadnego pliku"
  exit 1
}
echo "$outputs" | grep -q '^channels_high_dump' || {
  echo "regula DUMP nie zostawila zrzutu; pliki: $outputs"
  exit 1
}
for file in $outputs; do
  bash ../compare.sh "run_literal/$file" "run_let/$file"
done
extra=$(cd run_let && ls | grep -v -e '^plan.rql$' -e '^data.txt$' -e '^dump\.')
[ "$extra" = "$outputs" ] || {
  echo "rozne zestawy plikow: literal [$outputs], let [$extra]"
  exit 1
}
echo OK
