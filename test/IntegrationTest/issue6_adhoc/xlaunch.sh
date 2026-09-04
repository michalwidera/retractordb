#!/bin/bash
# Tryb zapytania ad hoc (issue #6): xqry -a definiuje strumien w locie.
#
# Synchronizacja jest ZDARZENIOWA, nie czasowa. Poprzednia wersja czekala na sam
# fakt istnienia pliku blokady (co spelniala rowniez blokada CUDZEGO serwera)
# i odmierzala `sleep 0.1` do `xqry -k`, po czym kasowala plik blokady — takze
# nie swoj. Wspolna oprawa ../serverlib.sh zalatwia start, koniec i higiene;
# tutaj zostaje to, co specyficzne dla ad hoc: sprawdzenie wyniku `xqry -a`
# (xqry pisze diagnostyke na stdout, nie stderr) i oczekiwanie na artefakty.
set -e
. "$(dirname "$0")/../serverlib.sh"

xretractor query-6.rql -c
server_start query-6.rql

adhoc_out=$(xqry -a 'select * stream stradhoc from core0' 2>&1) || {
  echo "ad hoc: zapytanie odrzucone: $adhoc_out"
  exit 1
}

# Artefakty powstaja po powrocie z xqry -a; czekamy na nie zamiast na zegar.
for _ in $(seq 1 200); do
  if [ -s temp/stradhoc ] && [ -s temp/stradhoc.desc ] && [ -s temp/stradhoc.meta ]; then
    break
  fi
  sleep 0.05
done
if ! { [ -s temp/stradhoc ] && [ -s temp/stradhoc.desc ] && [ -s temp/stradhoc.meta ]; }; then
  echo "ad hoc: brak artefaktow strumienia stradhoc; temp zawiera:"
  ls -la temp
  echo "ad hoc: odpowiedz xqry -a: $adhoc_out"
  exit 1
fi

xqry -k
server_wait_exit
