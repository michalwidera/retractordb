#!/bin/bash
# Sprawdzenie jednego przypadku faktoryzacji przesuniec pod HASH-em (issue #202).
#
# Logika wyniesiona z CMakeLists do skryptu, a nie zapisana jako `bash -c "..."`:
# makro add_test przekazuje argumenty przez `_add_test(${ARGV})`, ktore tnie polecenie
# po jego WEWNETRZNYCH srednikach, a konstrukcji `if grep ... ; then exit 1 ; fi`
# nie da sie zapisac bez srednika. Regula domowa jest w harness_command_integrity.py:
# skrypt z wieloma poleceniami idzie do wlasnego pliku.
set -e

name="$1"
[ -n "$name" ] || {
  echo "uzycie: $0 <matched|unmatched|zero>"
  exit 2
}

out="$name.out"
xretractor "$name.rql" -c >"$out"

# Napis MUSI wystapic. Bez -q, tak jak w poprzedniej wersji: przy `ctest -V`
# dopasowana linia jest widoczna i od razu widac, co kompilator wypisal.
must() {
  grep -F "$1" "$out" || {
    echo "brak oczekiwanego napisu: $1"
    exit 1
  }
}

# Napis NIE MOZE wystapic — to jest wlasnie ta polowa asercji, ktora wymuszala `if`.
must_not() {
  if grep -F "$1" "$out"; then
    echo "napis nie powinien wystapic: $1"
    exit 1
  fi
}

case "$name" in
matched)
  must 'matched(1/15)'
  must ':- PUSH_STREAM(STREAM_HASH_A_B)'
  must ':- STREAM_TIMEMOVE(3)'
  must 'STREAM_HASH_A_B(1/15)'
  must_not 'STREAM_TIMEMOVE_2_A('
  must_not 'STREAM_TIMEMOVE_1_B('
  ;;
unmatched)
  must 'unmatched(1/15)'
  must ':- PUSH_STREAM(STREAM_TIMEMOVE_1_A)'
  must ':- PUSH_STREAM(STREAM_TIMEMOVE_1_B)'
  must 'STREAM_TIMEMOVE_1_A(1/10)'
  must 'STREAM_TIMEMOVE_1_B(1/5)'
  must_not 'STREAM_HASH_A_B('
  ;;
zero)
  must 'zero(1/20)'
  must ':- PUSH_STREAM(STREAM_HASH_A_B)'
  must 'STREAM_HASH_A_B(1/20)'
  must_not 'STREAM_TIMEMOVE('
  ;;
*)
  echo "nieznany przypadek: $name"
  exit 2
  ;;
esac
