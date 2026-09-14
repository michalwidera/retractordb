#!/bin/bash
# WARTOSC: `(x+1)+2` nad slotem FLOAT = 2^24 daje to samo bez wzgledu na RDB_OPT_SIMPLIFY_EXPRESSIONS.
#
# Przebieg OFFLINE (`-f`) z budzetem slotow: liczba rekordow nie zalezy od zegara.
# Wartosci czytamy z ARTEFAKTOW, nie przez klienta.
set -e
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -r -m 4

# FLOAT 2^24 gubi +1: (x+1)+2 = 16777218, przepisane x+3 = 16777220. Deskryptor deklaruje dzis oba pola
# jako INTEGER (ksztaltu wyjscia wezlow syntetyzujacych inferFieldShapes() nie rusza), wiec rekord to dwa
# INTEGER, 8 bajtow. Gdy ksztalt wyjscia zostanie poprawiony, zmieni sie tu uklad rekordu, nie wartosc.
expected="16777218 16777218"

for stream in w r; do
  bytes=($(od -An -tu1 -v temp/$stream))
  test ${#bytes[@]} -gt 0 && test $((${#bytes[@]} % 8)) = 0 || {
    echo "REGRESJA: rozmiar '$stream' (${#bytes[@]} B) nie jest dodatnia wielokrotnoscia rekordu 8 B"
    cat temp/$stream.desc
    exit 1
  }
  int32() { echo $((bytes[$1] + (bytes[$1 + 1] << 8) + (bytes[$1 + 2] << 16) + (bytes[$1 + 3] << 24))); }
  for ((i = 0; i < ${#bytes[@]}; i += 8)); do
    actual="$(int32 $i) $(int32 $((i + 4)))"
    test "$actual" = "$expected" || {
      echo "REGRESJA: zle wartosci '$stream' w rekordzie $((i / 8))"
      echo "oczekiwane: $expected"
      echo "jest:       $actual"
      cat temp/$stream.desc
      exit 1
    }
  done
done
