#!/bin/bash
# Blad krytyczny ma konczyc proces CZYSTO: kodem EXIT_FAILURE, po wypisaniu diagnostyki
# i po wykonaniu sprzatania zarejestrowanego przez std::atexit.
#
# Do 2026-08-30 nie konczyl. Dwie rozne sciezki wywracaly proces JUZ PO wypisaniu
# wlasciwego komunikatu, przez co diagnostyka ginela za sladem awarii, a sprzatanie IPC
# sie nie wykonywalo:
#
#   1. FatalError wolal spdlog::shutdown() przed std::exit, a std::exit uruchamia handlery
#      atexit — executorsm::cleanup() zaczyna od SPDLOG_WARN. Po shutdown() rejestr jest
#      pusty, wiec makro wolalo should_log() na wskazniku zerowym: SIGSEGV, kod 139.
#
#   2. cleanup() robil bt.join() takze wtedy, gdy sam biegl w watku komunikacyjnym —
#      a biegnie tam, bo getAdHoc() wola compile(), a kompilator ma wiele wywolan
#      FatalError. join() na watku biezacym rzuca std::system_error, wyjatek z handlera
#      atexit to std::terminate: SIGABRT, kod 134.
#
# Test sprawdza OBIE sciezki po kodzie wyjscia, bo to jedyna wielkosc, ktora odroznia
# zakonczenie czyste (1) od segfaultu (139) i od abortu (134).
set -e
. "$(dirname "$0")/../serverlib.sh"

# --- Sciezka 1: blad krytyczny przy starcie, jeszcze przed uruchomieniem serwera. ---
# Katalog z dyrektywy STORAGE nie istnieje i RetractorDB go nie tworzy.
rm -rf ./nosuchdir
status=0
xretractor missing_storage.rql -m 4 -f >/dev/null 2>&1 || status=$?
if [ "$status" -ne 1 ]; then
  echo "start z brakujacym katalogiem STORAGE: kod wyjscia $status, oczekiwano 1"
  echo "  (139 = SIGSEGV w atexit, 134 = SIGABRT — obie znacza regresje sciezki wyjscia)"
  exit 1
fi

# --- Sciezka 2: blad krytyczny w WATKU KOMUNIKACYJNYM, przy zapytaniu ad hoc. ---
# `@(0,4)` ma krok zerowy, co kompilator odrzuca przez FatalError.
rm -rf ./temp && mkdir -p ./temp
rm -f ./*.desc ./*.meta ./*.shadow
xretractor query.rql -c >/dev/null
server_start query.rql -m 400 -k -r

xqry -a 'select * stream bad from src@(0,4)' >/dev/null 2>&1 || true

status=$(server_wait_status)
if [ "$status" -ne 1 ]; then
  echo "blad krytyczny w watku komunikacyjnym: kod wyjscia $status, oczekiwano 1"
  echo "  (134 = SIGABRT z join() na watku biezacym albo z destruktora std::thread)"
  exit 1
fi

# Blokada uslugi ma znikac SAMA. std::exit nie uruchamia destruktorow obiektow
# automatycznych, wiec FlockServiceGuard::~FlockServiceGuard() przy bledzie krytycznym
# sie nie wykonuje — plik kasuje executorsm::cleanup() zarejestrowany przez atexit.
# Bramka higieny w serverlib.sh sprawdza to za nas i oblewa test, jesli blokada zostanie;
# ten komentarz stoi tu po to, zeby bylo wiadomo, ze jej milczenie JEST asercja.

echo "OK"
