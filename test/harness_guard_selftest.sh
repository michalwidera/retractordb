#!/bin/bash
# Samotest straznika: straznik, ktory nie umie oblac, niczego nie pilnuje.
#
# Probka ma po jednym rozbitym tescie na kazdy zapis, ktory straznik musi
# rozpoznac (nazwa w cudzyslowie, nakladka, laczone i poprzedzone flagi, powloka
# spoza dawnej listy), oraz zdrowe pulapki. Zadamy, zeby straznik skonczyl
# kodem 1 i wskazal DOKLADNIE rozbite po nazwie.
#
# Probka lezy w repo jako harness_guard_sample.txt, a nie jako CTestTestfile.cmake:
# katalog test/ jest kopiowany do drzewa builda, wiec plik o tej drugiej nazwie
# trafilby pod skan prawdziwego straznika i ten oskarzylby sam siebie.
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
GUARD="$HERE/harness_command_integrity.py"
# Warstwa przenosnosci powloki; kopia test/ zachowuje ten uklad katalogow.
. "$HERE/IntegrationTest/portable.sh"

work=$(make_temp_dir)
trap 'rm -rf "$work"' EXIT
cp "$HERE/harness_guard_sample.txt" "$work/CTestTestfile.cmake"

out=$(python3 "$GUARD" "$work" 2>&1) && rc=0 || rc=$?

[ "$rc" = 1 ] || {
  echo "straznik nie oblal na probce z rozbitym testem (rc=$rc)"
  echo "$out"
  exit 1
}
for name in it_rozbity it_rozbity_cudzyslow it_rozbity_nakladka it_rozbity_ec \
  it_rozbity_pipefail it_rozbity_rcfile it_rozbity_dash; do
  grep -q "test '$name'" <<<"$out" || {
    echo "straznik nie wskazal rozbitego testu $name"
    echo "$out"
    exit 1
  }
done
for name in it_zdrowy it_jednoargumentowy it_zdrowy_xretractor it_zdrowy_skrypt; do
  if grep -q "test '$name'" <<<"$out"; then
    echo "straznik oskarzyl zdrowy test $name"
    echo "$out"
    exit 1
  fi
done
echo OK
