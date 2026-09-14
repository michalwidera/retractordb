#!/bin/bash
# WARTOSC: `(x+1)+2` nad slotem FLOAT = 2^24 daje to samo bez wzgledu na RDB_OPT_SIMPLIFY_EXPRESSIONS.
#
# Przebieg OFFLINE (`-f`) z budzetem slotow: liczba rekordow nie zalezy od zegara.
# Wartosci czytamy z ARTEFAKTOW, nie przez klienta.
set -e
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -r -m 4

# FLOAT 2^24 gubi +1: (x+1)+2 = 16777218, przepisane x+3 = 16777220. Oba pola sa FLOAT, bo typ wyjscia
# wezla syntetyzujacego to typ slotu wejsciowego (it_synth_node_output_shape) — rekord ma 8 bajtow.
# Do 2026-09-14 deskryptor deklarowal oba pola jako INTEGER; zmienil sie uklad rekordu, nie wartosc.
expected="16777218 16777218"

for stream in w r; do
  # `%.17g` zdejmuje z liczb dopelnienie formatu `od`; 2^24+2 jest w FLOAT dokladne.
  values=($(od -An -v -tf4 temp/$stream | awk '{ for (i = 1; i <= NF; i++) printf "%.17g\n", $i }'))
  test ${#values[@]} -gt 0 && test $((${#values[@]} % 2)) = 0 || {
    echo "REGRESJA: rozmiar '$stream' (${#values[@]} wartosci FLOAT) nie jest dodatnia wielokrotnoscia rekordu 2 x FLOAT"
    cat temp/$stream.desc
    exit 1
  }
  for ((i = 0; i < ${#values[@]}; i += 2)); do
    actual="${values[i]} ${values[i + 1]}"
    test "$actual" = "$expected" || {
      echo "REGRESJA: zle wartosci '$stream' w rekordzie $((i / 2))"
      echo "oczekiwane: $expected"
      echo "jest:       $actual"
      cat temp/$stream.desc
      exit 1
    }
  done
done
