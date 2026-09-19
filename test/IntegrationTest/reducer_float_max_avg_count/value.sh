#!/bin/bash
# WARTOSC: MAX nad FLOAT/DOUBLE i AVG nad wiecej niz 255 polami licza sie poprawnie.
#
# Przebieg OFFLINE (`-f -u`): dwa wiersze danych to dwa rekordy, niezaleznie od zegara.
# Wartosci czytamy z ARTEFAKTOW przez `xtrdb list`.
# Do 2026-09-14: fmax = 1.17549e-38, dmax = 2.22507e-308, a avg256 przerywal silnik
# wyjatkiem `bad rational: zero denominator`.
set -e
. "$(dirname "$0")/../portable.sh"
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -u

check() {
  local stream=$1 records=$2 expected=$3
  local lines
  # `xtrdb` otwiera artefakt z katalogu biezacego; spoza `temp` czeka w nieskonczonosc na wejscie.
  lines=$(cd temp && printf 'open %s\nlist 10\nquit\n' "$stream" | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
  local count
  count=$(printf '%s\n' "$lines" | grep -c -F '{' || true)
  test "$count" = "$records" || {
    echo "REGRESJA: '$stream' ma $count rekordow, oczekiwane $records"
    printf '%s\n' "$lines"
    exit 1
  }
  while IFS= read -r line; do
    test "$line" = "$expected" || {
      echo "REGRESJA: zle wartosci '$stream'"
      echo "oczekiwane: $expected"
      echo "jest:       $line"
      exit 1
    }
  done <<< "$lines"
}

# Maksimum samych ujemnych wartosci; MIN jest kontrola, ktora dzialala zawsze.
check fmax 2 '{ max:-3.5 }'
check fmin 2 '{ min:-5.5 }'
check dmax 2 '{ max:-2.25 }'

# Srednia 1..256 = 32896/256 = 257/2; srednia 1..257 = 33153/257 = 129.
check avg256 2 '{ avg:257/2 }'
check avg257 2 '{ avg:129/1 }'
