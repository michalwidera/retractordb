#!/usr/bin/env bash
# Generator strumieni koncem-do-konca.
#
# Twierdzenie: `SELECT cells[$] STREAM cell[4] FROM cells` daje DOKLADNIE to samo, co
# cztery recznie rozpisane SELECT-y. Sprawdzane na dwa sposoby, bo kazdy lapie co innego:
#   1) wyniki obu wariantow rowne co do bitu — generator nie zmienia NICZEGO w wykonaniu;
#   2) wartosci zgodne z definicja zrodla — oba warianty nie moga byc zgodnie bledne.
#
# Test biegnie przez pelna sciezke uruchomieniowa, nie przez sam kompilator, i to jest jego
# powod istnienia: pierwsza wersja generatora kompilowala sie do planu nie do odroznienia od
# recznego, a mimo to przerywala start, bo launcher mapowal linie RQL na jeden strumien.
# Testy jednostkowe planu takiego defektu zobaczyc nie moga.
set -e
rm -rf gen man
for variant in gen man; do
  mkdir -p "$variant"
  cp cells.txt "$variant/"
done
cp query.rql gen/
cp manual.rql man/

# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten sam.
# Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera -f
# znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
(cd gen && xretractor query.rql -r -k -m 4 -f >/dev/null)
(cd man && xretractor manual.rql -r -k -m 4 -f >/dev/null)

dump() { od -An -v -tu4 "$1" | xargs; }

for i in 0 1 2 3; do
  name='cell$'"$i"

  [ -s "gen/$name" ] || {
    echo "generator nie wytworzyl strumienia $name"
    exit 1
  }

  cmp -s "gen/$name" "man/$name" || {
    echo "$name: generator rozni sie od recznego zapisu"
    echo "  generator: $(dump "gen/$name")"
    echo "  recznie:   $(dump "man/$name")"
    exit 1
  }

  # cells.txt to 1..16, pole ma 4 elementy, wiec rekord k niesie 4k+1 .. 4k+4,
  # a strumien cell$i to kolejne 4k+i+1. Wzorzec przyciety do liczby rekordow.
  records=$(($(stat -c %s "gen/$name") / 4))
  expected=$(for k in $(seq 0 $((records - 1))); do echo $((4 * k + i + 1)); done | xargs)
  [ "$(dump "gen/$name")" = "$expected" ] || {
    echo "$name: wartosci niezgodne z definicja zrodla"
    echo "  oczekiwano: $expected"
    echo "  otrzymano:  $(dump "gen/$name")"
    exit 1
  }
done

echo OK
