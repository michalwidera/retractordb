#!/bin/bash
# covers: rqlparser.parser-rql-string.17~1, streaminstance.stream-instance.11~1, datamodel.data-model.17~1, datamodel.data-model.16~1
# Kazdy plan od startu dwa razy; porownanie wartosci i NULL niezalezna wyrocznia,
# a artefaktow drugiego przebiegu bajtowo przez compare.sh. Opakowanie poprzedza
# kazdy proces silnika, wiec test -vg- naprawde wykonuje caly scenariusz pod Valgrindem.
set -euo pipefail
scenario="$1"
engine="$2"
shift 2
here=$(cd "$(dirname "$0")" && pwd)
case_name=${scenario%-restart}
cd "$here/$case_name"

if [ "$scenario" = "uc10-restart" ]; then
  rm -rf restart
  mkdir -p restart/temp
  cp query.rql hour.txt restart/
  cp restart_before.txt restart/temp.txt
  if ! (cd restart && "$@" "$engine" query.rql -k -f -u > before.log 2>&1); then
    cat restart/before.log
    exit 1
  fi
  # Drugi proces startuje na tym samym magazynie; kopia zachowuje dowod pierwszego.
  cp -r restart/temp restart/before
  cp restart_after.txt restart/temp.txt
  if ! (cd restart && "$@" "$engine" query.rql -k -r -f -u > after.log 2>&1); then
    cat restart/after.log
    exit 1
  fi
  python3 "$here/oracle.py" restart "$PWD" restart/before restart/temp
  exit
fi

for run in 1 2; do
  rm -rf temp "run$run"
  mkdir temp
  if ! "$@" "$engine" query.rql -k -f -u > "run$run.log" 2>&1; then
    cat "run$run.log"
    exit 1
  fi
  mv temp "run$run"
  python3 "$here/oracle.py" check "$PWD" "run$run"
done

# Porownujemy tylko artefakty wyjscia, zdefiniowane i sprawdzone przez wyrocznie.
python3 "$here/oracle.py" files "$PWD" > artifacts.txt
while IFS= read -r name; do
  bash "$here/../compare.sh" "run1/$name" "run2/$name"
done < artifacts.txt
