#!/bin/bash
# Niepoprawny token liczbowy w TEXTFILE (#471): `abc`, `12x` i `99999999999` w polu INTEGER daja NULL
# zamiast 0, obcietej liczby i wartosci granicznej; `+7` i `-5` zostaja liczbami. Sasiednie pole
# rekordu i kolejne rekordy czytaja sie bez przesuniecia. Kod wyjscia 0 i dokladnie jedno ostrzezenie
# w dzienniku silnika - mimo trzech zlych tokenow.
#
# Argumenty: sciezka xretractor z drzewa buildu, CMAKE_BUILD_TYPE. W Release SPDLOG_WARN jest
# wycinany w kompilacji, wiec tam ostrzezenia nie ma i jego liczba nie jest sprawdzana.
set -e
. "$(dirname "$0")/../portable.sh"
export LC_ALL=C
XRETRACTOR="$1"
BUILD_TYPE="$2"

# Dziennik silnika lezy w $TMPDIR i dzieli go slot przestrzeni nazw - liczymy tylko przyrost.
log="${TMPDIR:-/tmp}/xretractor.log"
offset=0
[ -f "$log" ] && offset=$(wc -c <"$log")

rm -rf temp
mkdir temp
# `-f -u`: offline do wyczerpania zrodla - 5 wierszy danych to 5 rekordow.
"$XRETRACTOR" bad.rql -k -f -u > /dev/null
(cd temp && printf 'open dst\nlist 1000\nquit\n' | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true) > bad.out
bash ../compare.sh --ignore-eol bad.pattern bad.out

[ "$BUILD_TYPE" = "Release" ] && exit 0
warnings=$(tail -c +$((offset + 1)) "$log" 2>/dev/null | grep -cF "invalid numeric token" || true)
if [ "$warnings" != "1" ]; then
  echo "oczekiwano 1 ostrzezenia w dzienniku, jest: $warnings"
  tail -c +$((offset + 1)) "$log" | tail -20
  exit 1
fi
