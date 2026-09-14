#!/bin/bash
# WARTOSC: `merged[2]` niesie `core1.c` w calosci, takze powyzej 255.
#
# Przebieg OFFLINE (`-f`) z budzetem 12 slotow: liczba rekordow nie zalezy od zegara.
# Wartosci czytamy z ARTEFAKTU, nie przez klienta.
set -e
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -r -m 12

# Rekord = BYTE, INTEGER, BYTE, INTEGER, bez wyrownania: 10 bajtow. `core1` trzyma wartosc przez
# dwa sloty `merged`. Do 2026-09-14 400 w `merged_1` zapisywalo sie po cichu jako 144 (0x190 -> 0x90).
expected="1 100 1 100 2 100 2 100 3 200 3 200 4 200 4 200 5 300 5 300 6 300 6 300 1 400 1 400 2 400 2 400 3 100 3 100 4 100 4 100"

bytes=($(od -An -tu1 -v temp/merged))
test $((${#bytes[@]} % 10)) = 0 || {
  echo "REGRESJA: rozmiar 'merged' (${#bytes[@]} B) nie jest wielokrotnoscia rekordu 10 B"
  cat temp/merged.desc
  exit 1
}
int32() { echo $((bytes[$1] + (bytes[$1 + 1] << 8) + (bytes[$1 + 2] << 16) + (bytes[$1 + 3] << 24))); }
values=()
for ((i = 0; i < ${#bytes[@]}; i += 10)); do
  values+=("${bytes[i]}" "$(int32 $((i + 1)))" "${bytes[i + 5]}" "$(int32 $((i + 6)))")
done
actual="${values[*]}"
test "$actual" = "$expected" || {
  echo "REGRESJA: zle wartosci 'merged'"
  echo "oczekiwane: $expected"
  echo "jest:       $actual"
  cat temp/merged.desc
  exit 1
}
