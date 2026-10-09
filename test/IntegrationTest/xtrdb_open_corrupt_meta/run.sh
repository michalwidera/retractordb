#!/bin/bash
# `open` na uszkodzonym `.meta` / `.meta.shadow` odmawia i nie przerywa xtrdb (#421).
#
# Wpis indeksu: flaga (1 B) + licznik (8 B) + bitsetSize (8 B) + bitset. Plik `.meta` ma przed
# wpisami 8-bajtowy naglowek, `.meta.shadow` nie ma naglowka. bitsetSize = SIZE_MAX-6 zawijal
# zaokraglenie packedByteCount do 0 i omijal straznika ramki.
#
# Uzycie: run.sh <xtrdb>
set -e
. "$(dirname "$0")/../portable.sh"
xtrdb="$1"
rm -f corr corr.* out.txt

# run_xtrdb <wejscie printf> - wynik w out.txt, kod wyjscia w $rc.
run_xtrdb() {
  set +e
  # shellcheck disable=SC2059 # wejscie jest formatem printf z \n
  printf "$1" | run_timeout 20 "$xtrdb" noprompt >out.txt 2>&1
  rc=$?
  set -e
}

expect() { # expect <opis> <wzorzec w out.txt>
  if ! grep -qF -- "$2" out.txt; then
    echo "$1: brak '$2' w wyjsciu (kod $rc):"
    cat out.txt
    exit 1
  fi
}

expect_rc0() { # expect_rc0 <opis>
  if [ "$rc" -ne 0 ]; then
    echo "$1: kod $rc zamiast 0:"
    cat out.txt
    exit 1
  fi
}

# corrupt_bitset_size <plik> <offset> - wpisuje SIZE_MAX-6 (little-endian) w pole bitsetSize.
corrupt_bitset_size() {
  printf '\371\377\377\377\377\377\377\377' | dd of="$1" bs=1 seek="$2" conv=notrunc 2>/dev/null
}

# (1) Kontrola dodatnia: magazyn z dwoma rekordami otwiera sie i widzi swoj rozmiar.
run_xtrdb 'open corr { INTEGER a }\nappend\nappend\nquit\n'
expect_rc0 "utworzenie magazynu"
run_xtrdb 'open corr\nsize\nquit\n'
expect_rc0 "open poprawnego magazynu"
expect "open poprawnego magazynu" "2 Record(s)"

# (2) Uszkodzony `.meta`: odmowa z komunikatem, polecenia po niej trafiaja na niepolaczony xtrdb.
cp corr.meta corr.meta.good
corrupt_bitset_size corr.meta $((8 + 9))
run_xtrdb 'open corr\nsize\nquit\n'
expect_rc0 "open z uszkodzonym .meta"
expect "open z uszkodzonym .meta" "open: Bitset size 18446744073709551609 exceeds remaining buffer"
expect "open z uszkodzonym .meta" "unconnected"

# (3) Uszkodzony `.meta.shadow`: nadpisanie rekordu tworzy cien, jego uszkodzenie tez daje odmowe.
cp corr.meta.good corr.meta
run_xtrdb 'open corr\nwrite 0\nquit\n'
expect_rc0 "nadpisanie rekordu"
if [ ! -s corr.meta.shadow ]; then
  echo "nadpisanie rekordu nie utworzylo corr.meta.shadow"
  cat out.txt
  exit 1
fi
corrupt_bitset_size corr.meta.shadow 9
run_xtrdb 'open corr\nsize\nquit\n'
expect_rc0 "open z uszkodzonym .meta.shadow"
expect "open z uszkodzonym .meta.shadow" "open: ShadowOverride bitset size 18446744073709551609 exceeds remaining buffer"
expect "open z uszkodzonym .meta.shadow" "unconnected"

echo "PASS"
