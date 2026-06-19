#!/bin/bash
# Scenariusz trybu usługowego/idle xretractor. Argument $1:
#   (brak) -> tryb flagi --service
#   env    -> tryb zmiennej środowiskowej XRETRACTOR_SERVICE=1
# Logikę trzymamy w skrypcie (nie w 'bash -c' w add_test), bo średniki w CMake
# są separatorem listy i rozbiłyby polecenie na osobne argumenty.
set -e

# Poczekaj aż ewentualna poprzednia instancja zwolni blokadę usługi.
while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done

if [ "$1" = "env" ]; then
  # Logowanie usługowe włączone zmienną środowiskową, bez flagi --service.
  XRETRACTOR_SERVICE=1 xretractor --noanykey -m 5 2>stderr.txt
else
  # Logowanie usługowe włączone flagą --service; start bez pliku .rql = tryb idle.
  xretractor --service --noanykey -m 5 2>stderr.txt
  grep -q 'No query file provided' stderr.txt
  grep -q 'Idle mode: no queries to process' stderr.txt
fi

# Format logu usługowego: 'poziom + treść' na stderr, bez znacznika czasu.
grep -qE '^\[I\] ' stderr.txt
