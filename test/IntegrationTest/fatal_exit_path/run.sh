#!/bin/bash
# Blad krytyczny ma konczyc proces CZYSTO: kodem EXIT_FAILURE, po wypisaniu diagnostyki
# i po wykonaniu sprzatania zarejestrowanego przez std::atexit.
#
# Do 2026-08-30 nie konczyl. Dwie rozne sciezki wywracaly proces JUZ PO wypisaniu
# wlasciwego komunikatu, przez co diagnostyka ginela za sladem awarii, a sprzatanie IPC
# sie nie wykonywalo:
#
#   1. FatalError wolal spdlog::shutdown() przed std::exit, a std::exit uruchamia handlery
#      atexit - executorsm::cleanup() zaczyna od SPDLOG_WARN. Po shutdown() rejestr jest
#      pusty, wiec makro wolalo should_log() na wskazniku zerowym: SIGSEGV, kod 139.
#
#   2. cleanup() robil bt.join() takze wtedy, gdy sam biegl w watku komunikacyjnym -
#      a biegnie tam, bo getAdHoc() wola compile(), a kompilator ma wiele wywolan
#      FatalError. join() na watku biezacym rzuca std::system_error, wyjatek z handlera
#      atexit to std::terminate: SIGABRT, kod 134.
#
# Test sprawdza OBIE sciezki po kodzie wyjscia, bo to jedyna wielkosc, ktora odroznia
# zakonczenie czyste (1) od segfaultu (139) i od abortu (134).
set -e
. "$(dirname "$0")/../portable.sh"
. "$(dirname "$0")/../serverlib.sh"

# --- Sciezka 1: blad krytyczny przy starcie, jeszcze przed uruchomieniem serwera. ---
# Brak katalogu STORAGE jest K2 od #473. Hak wywoluje K1 na poprawnym planie,
# po zarejestrowaniu cleanup, przed budowa modelu i uruchomieniem watku komunikacyjnego.
rm -rf ./temp && mkdir -p ./temp
status=0
RDB_FAULT_FATAL_AT_START=1 run_timeout 30 xretractor query.rql -m 4 -f >/dev/null 2>start.err || status=$?
if [ "$status" -ne 1 ]; then
  echo "blad krytyczny przy starcie: kod wyjscia $status, oczekiwano 1"
  echo "  (139 = SIGSEGV w atexit, 134 = SIGABRT - obie znacza regresje sciezki wyjscia)"
  exit 1
fi
grep -q 'FATAL: fault hook RDB_FAULT_FATAL_AT_START' start.err || {
  echo "blad krytyczny przy starcie: brak komunikatu haka"
  cat start.err
  exit 1
}
if [ -e "$SERVER_LOCK" ]; then
  lock_is_free "$SERVER_LOCK" || { echo "blad krytyczny przy starcie zostawil zajeta blokade"; exit 1; }
  rm -f "$SERVER_LOCK"
fi

# --- Sciezka 2: blad krytyczny w WATKU KOMUNIKACYJNYM, przy zapytaniu ad hoc. ---
# Do 2026-09-27 wywolywal go tekst `@(0,4)` - krok zerowy, odrzucany przez kompilator
# FatalError-em. Od #308 parser odmawia go wczesniej, wiec sciezke otwiera hak
# RDB_FAULT_FATAL_IN_ADHOC w miejscu lokalnej kompilacji; zapytanie jest poprawne.
rm -rf ./temp && mkdir -p ./temp
rm -f ./*.desc ./*.meta ./*.shadow
xretractor query.rql -c >/dev/null
export RDB_FAULT_FATAL_IN_ADHOC=1
server_start query.rql -m 400 -k -r 2>adhoc.err
unset RDB_FAULT_FATAL_IN_ADHOC

xqry -a 'select * stream fine from src@(1,4)' >/dev/null 2>&1 || true

status=$(server_wait_status)
if [ "$status" -ne 1 ]; then
  echo "blad krytyczny w watku komunikacyjnym: kod wyjscia $status, oczekiwano 1"
  echo "  (134 = SIGABRT z join() na watku biezacym albo z destruktora std::thread)"
  exit 1
fi
if ! grep -q "FATAL: fault hook RDB_FAULT_FATAL_IN_ADHOC" adhoc.err; then
  echo "blad krytyczny w watku komunikacyjnym: brak komunikatu haka na stderr:"
  cat adhoc.err
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
    run_timeout 30 xqry -t w >/dev/null 2>&1 &
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

# --- Sciezka 5: WYJATEK POZA granica bledu handlera komendy (#486). ---
#
# commandProcessor() ma wlasny catch(std::exception) i sam opisuje awarie klientowi -- pilnuje
# tego it_show_handler_failure przez hak RDB_FAULT_SHOW. Ta sciezka bada pietro NIZEJ: wyjatek,
# ktory przechodzi OBOK tego catch-a. Bez zapory wokol onCommand w IpcServer::commandLoop()
# lapal go dopiero catch calej wiadomosci: wpis "IPC message dropped" i zadnej odpowiedzi, wiec
# klient czekal w ciszy do limitu czasu na odpowiedz, ktora nigdy nie przychodzila.
#
# Hak RDB_FAULT_THROW_IN_COMMAND rzuca z lambdy .onCommand, czyli dokladnie tam: za granica
# handlera, przed transportem. Jest zawezony do NAZWY komendy, zeby 'kill' pozostalo osiagalne.
# Sprawdzamy caly kontrakt zapory: usluga zyje, klient dostaje POWOD (a nie cisze), kolejne
# komendy nadal dzialaja, a zatrzymanie jest regularne (kod 0).
rm -rf ./temp && mkdir -p ./temp
rm -f ./*.desc ./*.meta ./*.shadow
xretractor query.rql -c >/dev/null

# Log klienta jest wspolny dla przestrzeni nazw i DOPISYWANY -- bez oproznienia sprawdzenie
# ponizej lapaloby zdanie sasiada z tego samego slotu puli (patrz it_show_handler_failure).
QRY_LOG="${TMPDIR:-/tmp}/xqry.log"
: > "$QRY_LOG"

# Hak czyta wylacznie serwer; klient dostaje juz srodowisko bez niego, zeby bylo
# widac, ze powod przyszedl po linii IPC, a nie ze zmiennej.
export RDB_FAULT_THROW_IN_COMMAND=show
server_start query.rql -m 400 -k -r 2>command.err
unset RDB_FAULT_THROW_IN_COMMAND

rc=0
run_timeout 30 xqry -s w -m 2 >/dev/null 2>&1 || rc=$?

if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "wyjatek poza granica handlera zabil serwer - brak zapory w IpcServer::commandLoop()"
  exit 1
fi

# no_stream_resources = selectResult::clientQueueMissing. Kod NIE odroznia powodu od ciszy:
# brak odpowiedzi na 'show' klient tez melduje jako odmowe ("server not found"). Wyklucza
# tylko inne awarie klienta i limit run_timeout; sedno sprawdza grep ponizej.
expected_rc=$(errno_value ENOSR)
if [ "$rc" -ne "$expected_rc" ]; then
  echo "xqry po wyjatku w handlerze: kod $rc, oczekiwano $expected_rc; tresc $QRY_LOG:"
  cat "$QRY_LOG"
  exit 1
fi

if ! grep -F "command handler failure: RDB_FAULT_THROW_IN_COMMAND" "$QRY_LOG"; then
  echo "zapora zatrzymala wyjatek, ale klient nie dostal powodu; tresc $QRY_LOG:"
  cat "$QRY_LOG"
  exit 1
fi

# Jedno wadliwe zadanie nie jest powodem, zeby przestac obslugiwac pozostale.
if ! run_timeout 30 xqry -l >/dev/null 2>&1; then
  echo "serwer przezyl wyjatek, ale nie obsluguje juz kolejnych komend"
  exit 1
fi

xqry -k >/dev/null
# server_wait_status biegnie w podpowloce $(...), ktora zna kod tylko procesu juz zakonczonego.
if ! wait_for_exit "$_server_pid"; then
  echo "serwer zyje 30 s po 'kill' wyslanym po wyjatku w handlerze"
  exit 1
fi
status=$(server_wait_status)
if [ "$status" -ne 0 ]; then
  echo "zatrzymanie po wyjatku w handlerze: kod wyjscia $status, oczekiwano 0"
  cat command.err
  exit 1
fi

# Blokada uslugi ma znikac SAMA. std::exit nie uruchamia destruktorow obiektow
# automatycznych, wiec FlockServiceGuard::~FlockServiceGuard() przy bledzie krytycznym
# sie nie wykonuje - plik kasuje executorsm::cleanup() zarejestrowany przez atexit.
# Bramka higieny w serverlib.sh sprawdza to za nas i oblewa test, jesli blokada zostanie;
# ten komentarz stoi tu po to, zeby bylo wiadomo, ze jej milczenie JEST asercja.

echo "OK"
