#!/bin/bash
# WARTOSC: rekordy zapisane bez obciecia i bez przepelnienia.
#
# Przebieg OFFLINE (`-f`) z budzetem slotow: liczba rekordow nie zalezy od zegara.
# Wartosci czytamy z ARTEFAKTOW, nie przez klienta.
set -e
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -r -m 6

# Kazdy rekord strumienia ma dac `expected`. `od` z formatem `fmt` czyta caly plik, a `size` to
# liczba wartosci w rekordzie (RATIONAL = dwie wartosci int32: licznik i mianownik).
check() {
  local stream=$1 fmt=$2 size=$3 expected=$4
  # `%.17g` zdejmuje z liczb zmiennoprzecinkowych dopelnienie formatu `od` (`2.5000000`).
  local values=($(od -An -v -"$fmt" temp/$stream | awk '{ for (i = 1; i <= NF; i++) printf "%.17g\n", $i }'))
  test ${#values[@]} -gt 0 && test $((${#values[@]} % size)) = 0 || {
    echo "REGRESJA: rozmiar '$stream' (${#values[@]} wartosci $fmt) nie jest dodatnia wielokrotnoscia rekordu ($size)"
    cat temp/$stream.desc
    exit 1
  }
  for ((i = 0; i < ${#values[@]}; i += size)); do
    local actual="${values[*]:i:size}"
    test "$actual" = "$expected" || {
      echo "REGRESJA: zle wartosci '$stream' w rekordzie $((i / size))"
      echo "oczekiwane: $expected"
      echo "jest:       $actual"
      cat temp/$stream.desc
      exit 1
    }
  done
}

# B - do 2026-09-14: w = 2 2, av = 1 3 1, mx = 2 2.
check w tf4 2 "2.5 2.5"
check ws tf4 2 "2.5 2.5"
check av td4 6 "3 2 3 1 3 2"
check avs td4 2 "3 2"
check mx tf4 2 "2.5 2.5"
# C - do 2026-09-14: sb = 1705032705/2 (przepelniony rational<int>), st = 1/3.
check sb tf8 1 "3000000000.5"
check st tf8 1 "$(awk 'BEGIN { printf "%.17g", 0.333333333333 }')"
# D - do 2026-09-14: acc = 2.5 5 7.
check acc tf4 3 "2.5 5 7.5"
