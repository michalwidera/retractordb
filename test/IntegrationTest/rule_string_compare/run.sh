#!/bin/bash
# Liczba odpalen kazdej reguly: akcja SYSTEM (synchroniczne ::system()) dopisuje nazwe reguly do fired.txt.
set -e
mkdir -p temp
rm -f temp/* fired.txt
touch fired.txt

# Przebieg OFFLINE do wyczerpania zrodla (`-f -u`): cztery wiersze data.txt to cztery rekordy.
xretractor query.rql -k -f -u

for rule in eq lt ne never bare; do
  printf '%s %s\n' "$rule" "$(grep -cx "$rule" fired.txt || true)"
done > out.txt

bash ../compare.sh --ignore-eol pattern.txt out.txt
