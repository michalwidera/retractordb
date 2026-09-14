#!/bin/bash
# WARTOSC: `merged[3] + 1 + 2` i `core1[1] + 1 + 2` licza to samo, bez wzgledu na RDB_OPT_SIMPLIFY_EXPRESSIONS.
#
# Przebieg OFFLINE (`-f`) z budzetem slotow: liczba rekordow nie zalezy od zegara.
# Wartosci czytamy z ARTEFAKTU, nie przez klienta.
set -e
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -r -m 3

# Rekord = FLOAT, FLOAT, BYTE, INTEGER, bez wyrownania: 13 bajtow. Zrodla maja po jednym wierszu, wiec
# kazdy rekord jest taki sam. d = 2^24 to pierwsza wartosc, przy ktorej FLOAT gubi +1:
# (d+1)+2 = 16777218 (0x4b800001), a przepisane d+3 = 16777220 (0x4b800002).
expected="4b800001 4b800001 1 10"

bytes=($(od -An -tu1 -v temp/merged))
test ${#bytes[@]} -gt 0 && test $((${#bytes[@]} % 13)) = 0 || {
  echo "REGRESJA: rozmiar 'merged' (${#bytes[@]} B) nie jest dodatnia wielokrotnoscia rekordu 13 B"
  cat temp/merged.desc
  exit 1
}
hex32() { printf '%02x%02x%02x%02x' ${bytes[$1 + 3]} ${bytes[$1 + 2]} ${bytes[$1 + 1]} ${bytes[$1]}; }
int32() { echo $((bytes[$1] + (bytes[$1 + 1] << 8) + (bytes[$1 + 2] << 16) + (bytes[$1 + 3] << 24))); }
for ((i = 0; i < ${#bytes[@]}; i += 13)); do
  actual="$(hex32 $i) $(hex32 $((i + 4))) ${bytes[i + 8]} $(int32 $((i + 9)))"
  test "$actual" = "$expected" || {
    echo "REGRESJA: zle wartosci 'merged' w rekordzie $((i / 13))"
    echo "oczekiwane: $expected"
    echo "jest:       $actual"
    cat temp/merged.desc
    exit 1
  }
done
