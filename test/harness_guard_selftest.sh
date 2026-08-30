#!/bin/bash
# Samotest straznika: straznik, ktory nie umie oblac, niczego nie pilnuje.
#
# Probka ma jeden test rozbity i dwa zdrowe. Zadamy, zeby straznik skonczyl
# kodem 1 i wskazal DOKLADNIE ten rozbity po nazwie.
#
# Probka lezy w repo jako harness_guard_sample.txt, a nie jako CTestTestfile.cmake:
# katalog test/ jest kopiowany do drzewa builda, wiec plik o tej drugiej nazwie
# trafilby pod skan prawdziwego straznika i ten oskarzylby sam siebie.
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
GUARD="$HERE/harness_command_integrity.py"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp "$HERE/harness_guard_sample.txt" "$work/CTestTestfile.cmake"

out=$(python3 "$GUARD" "$work" 2>&1) && rc=0 || rc=$?

[ "$rc" = 1 ] || {
  echo "straznik nie oblal na probce z rozbitym testem (rc=$rc)"
  echo "$out"
  exit 1
}
grep -q "it_rozbity" <<<"$out" || {
  echo "straznik nie wskazal rozbitego testu po nazwie"
  echo "$out"
  exit 1
}
if grep -q "it_zdrowy\|it_jednoargumentowy" <<<"$out"; then
  echo "straznik oskarzyl zdrowy test"
  echo "$out"
  exit 1
fi
echo OK
