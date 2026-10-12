#!/bin/bash
# Suma strumienia z samym soba liczona offline jednym przejsciem przez dane.
# Wypis: rekordy strumieni `s1` (`a+a`, zrodlo deklarowane) i `s2` (`m+m`, zrodlo niedeklarowane)
# przez `xtrdb list` - kazdy rekord to dwa razy ten sam rekord skladnika.
set -e
. "$(dirname "$0")/../portable.sh"
export LC_ALL=C

rm -rf temp
mkdir temp
# `-f -u`: offline do wyczerpania zrodla - 5 wierszy danych to 5 rekordow.
xretractor self.rql -k -f -u > /dev/null
{
  for stream in s1 s2; do
    echo "== $stream"
    (cd temp && printf 'open %s\nlist 1000\nquit\n' "$stream" | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
  done
} > self.out
bash ../compare.sh --ignore-eol self.pattern self.out
