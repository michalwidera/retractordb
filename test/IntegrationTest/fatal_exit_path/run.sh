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

# --- Sciezki 3 i 4: blad krytyczny w SLOCIE przetwarzania. ---
# Do 2026-09-14 proces w ogole sie nie konczyl. dataModel::processRows() trzyma core_mutex,
# a executorsm::run() plan_epoch_mutex przez caly slot. FatalError wola std::exit, ktory
# uruchamia cleanup() w TYM SAMYM watku, a cleanup():
#   3. bral core_mutex -- watek czekal na muteks, ktory sam trzymal (bez klienta);
#   4. dolaczal watek komunikacyjny, ktory stal w handlerze komendy na plan_epoch_mutex
#      (z klientem, ktorego komenda trafila w slot).
# Obie sytuacje wisialy do SIGKILL, z niezwolnionym IPC i blokada uslugi. Po W3 zadne znane
# RQL nie prowadzi do bledu krytycznego w slocie, stad hak RDB_FAULT_FATAL_IN_SLOT: wypisuje
# znacznik po wzieciu blokad, czeka zadane ms i wola FatalError. Na znacznik czekamy, zeby
# komenda klienta na pewno trafila w slot, a nie przed niego.
wait_for_exit() {
  local pid="$1" left=300
  while [ "$left" -gt 0 ]; do
    kill -0 "$pid" 2>/dev/null || return 0
    sleep 0.1
    left=$((left - 1))
  done
  return 1
}

fatal_in_slot() {
  local label="$1" withClient="$2" i=0 clientPid=""
  rm -rf ./temp && mkdir -p ./temp
  rm -f ./*.desc ./*.meta ./*.shadow ./slot.err
  export RDB_FAULT_FATAL_IN_SLOT=3000
  server_start query.rql -k -r 2>slot.err
  unset RDB_FAULT_FATAL_IN_SLOT

  while ! grep -q "RDB_FAULT_FATAL_IN_SLOT: slot locked" slot.err 2>/dev/null; do
    i=$((i + 1))
    if [ "$i" -gt 300 ]; then
      echo "$label: hak nie zglosil wejscia w slot w ciagu 30 s"
      exit 1
    fi
    sleep 0.1
  done

  if [ "$withClient" = "client" ]; then
    timeout 30 xqry -t w >/dev/null 2>&1 &
    clientPid=$!
  fi

  if ! wait_for_exit "$_server_pid"; then
    echo "$label: serwer zyje 30 s po wejsciu w slot z bledem krytycznym (zawieszone wyjscie)"
    exit 1
  fi
  status=$(server_wait_status)
  [ -n "$clientPid" ] && { wait "$clientPid" 2>/dev/null || true; }
  if [ "$status" -ne 1 ]; then
    echo "$label: kod wyjscia $status, oczekiwano 1"
    exit 1
  fi
  if ! grep -q "FATAL: fault hook RDB_FAULT_FATAL_IN_SLOT" slot.err; then
    echo "$label: brak komunikatu bledu krytycznego na stderr:"
    cat slot.err
    exit 1
  fi
}

fatal_in_slot "blad krytyczny w slocie bez klienta" ""
fatal_in_slot "blad krytyczny w slocie z komenda klienta na blokadzie epoki" client

# Blokada uslugi ma znikac SAMA. std::exit nie uruchamia destruktorow obiektow
# automatycznych, wiec FlockServiceGuard::~FlockServiceGuard() przy bledzie krytycznym
# sie nie wykonuje — plik kasuje executorsm::cleanup() zarejestrowany przez atexit.
# Bramka higieny w serverlib.sh sprawdza to za nas i oblewa test, jesli blokada zostanie;
# ten komentarz stoi tu po to, zeby bylo wiadomo, ze jej milczenie JEST asercja.

echo "OK"
