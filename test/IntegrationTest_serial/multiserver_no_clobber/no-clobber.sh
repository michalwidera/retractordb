#!/bin/bash
# Regresja: druga instancja xretractor nie ma prawa skasowac obiektow IPC dzialajacego serwera.
#
# Defekt (naprawiony przestawieniem kolejnosci w executorsm::run): blokada instancji byla brana
# PO starcie transportu IPC, a commandLoop() na wejsciu kasuje segment, kolejke komend i muteks
# nazwany — sprzatajac po poprzedniku, ktory padl. Druga instancja zdazyla wiec skasowac te
# obiekty ZANIM odkryla, ze blokada jest zajeta. Skutki byly dwa i oba ciche: klienci pierwszego
# serwera dostawali "server not found" (po pelnym budzecie 3 s), a handler atexit intruza kasowal
# te same obiekty po raz drugi, juz po jego wyjsciu.
#
# Test pilnuje wlasnie tego, czego zaden istniejacy nie sprawdzal: ze OFIARA przezyla.
set -e

. "$(dirname "$0")/../serverlib.sh"

server_start --noanykey --xqrywait

# Warunek wstepny: serwer A obsluguje klientow. Sygnalem jest KOD WYJSCIA --
# qry::hello() przy powodzeniu nie wypisuje niczego, tylko zwraca 0.
xqry --hello

# Druga instancja przy zajetej blokadzie: ma odpasc, nie dotykajac cudzego IPC.
set +e
xretractor --noanykey --xqrywait </dev/null > intruder.txt 2>&1
intruder_rc=$?
set -e

if [ "$intruder_rc" -eq 0 ]; then
  echo "druga instancja wystartowala mimo zajetej blokady (rc=0)"
  cat intruder.txt
  exit 1
fi

# ISTOTA TESTU: serwer A nadal odpowiada. Przed naprawa konczylo sie to tutaj bledem
# "server not found" po pelnym budzecie oczekiwania na odpowiedz.
set +e
xqry --hello > hello_after.txt 2>&1
hello_rc=$?
set -e
if [ "$hello_rc" -ne 0 ]; then
  echo "serwer A przestal odpowiadac po probie startu drugiej instancji (rc=$hello_rc):"
  cat hello_after.txt
  exit 1
fi

# Blokada nadal nalezy do serwera A — intruz nie skasowal jej pliku przy wyjsciu.
grep -q '^MODE: ' "$SERVER_LOCK"

xqry -k
server_wait_exit
