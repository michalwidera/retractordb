#!/usr/bin/env bash
# Zmienne VAR bez SET liczy sie tak samo jak stale LET (#531, etap A): plan z VAR nie zwija zmiennych
# do liczb - czyta je ewaluator w kazdym takcie - a mimo to daje bajtowo to samo wyjscie.
#
#   1. Zrzut `-c` planu z VAR niesie tokeny PUSH_VAR, wiec porownanie nizej nie porownuje dwoch
#      planow literalowych; ani plan z VAR, ani plan z LET nie dostaje ostrzezen.
#   2. Ten sam przebieg na tych samych danych daje bajtowo rowne pliki strumieni, deskryptory
#      i zrzuty obu regul DUMP.
#
# Oba plany biegna pod ta sama nazwa pliku w osobnych katalogach, zeby sciezki w wyjsciu byly rowne.
set -e
here=$(pwd)
rm -rf run_let run_var
for variant in let var; do
  mkdir "run_$variant"
  cp "$variant.rql" "run_$variant/plan.rql"
  cp data.txt "run_$variant/"
done

for variant in let var; do
  (cd "run_$variant" && xretractor plan.rql -c >dump.txt 2>dump.err)
done
grep -q 'PUSH_VAR(GAIN\[0\])' run_var/dump.txt || {
  echo "zrzut planu z VAR nie zawiera odczytu zmiennej:"
  cat run_var/dump.txt
  exit 1
}
for variant in let var; do
  if grep -q 'warning' "run_$variant/dump.err"; then
    echo "plan $variant dal ostrzezenie:"
    cat "run_$variant/dump.err"
    exit 1
  fi
done
rm run_let/dump.txt run_let/dump.err run_var/dump.txt run_var/dump.err

# Flaga -f (--no-clock) poza trybem -c zdejmuje czekanie na zegar scienny.
(cd run_let && xretractor plan.rql -r -k -m 30 -f)
(cd run_var && xretractor plan.rql -r -k -m 30 -f)
cd run_let
outputs=$(ls | grep -v -e '^plan.rql$' -e '^data.txt$')
cd "$here"
for dump in channels_high_dump scaled_low_dump; do
  echo "$outputs" | grep -q "^$dump" || {
    echo "regula $dump nie zostawila zrzutu; pliki: $outputs"
    exit 1
  }
done
for file in $outputs; do
  bash ../compare.sh "run_let/$file" "run_var/$file"
done
extra=$(cd run_var && ls | grep -v -e '^plan.rql$' -e '^data.txt$')
[ "$extra" = "$outputs" ] || {
  echo "rozne zestawy plikow: let [$outputs], var [$extra]"
  exit 1
}
echo OK
