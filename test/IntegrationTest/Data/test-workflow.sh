#!/bin/bash
# Przebieg uzytkowy: kompilacja, start serwera, trzy zapytania przez IPC,
# zatrzymanie i kontrola, ze kolejki w /dev/shm nie zostaly po nas.
#
# Poprzednia wersja zaczynala od `pkill xretractor`, czyli ubijala KAZDA
# instancje w systemie — takze cudza, nie swoja. Za to, zeby poprzedni test nie
# zostawil po sobie serwera, odpowiada teraz bramka higieny w ../serverlib.sh:
# obarcza winowajce zamiast pozwalac mu sprzatac po sobie cudzymi rekami.
set -e

if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
  echo "Usage: $0 <query.rql> <stream1> <stream2>"
  exit 1
fi

. "$(dirname "$0")/../serverlib.sh"

rm -f core*
rm -f str*

# Kolejki odpowiedzi TEJ przestrzeni nazw, a nie wszystkie w systemie. Nazwa kolejki to
# "brcdbr.<instancja>.<klient>" (ipc::names w constants.hpp), wiec globalne brcdbr* liczy
# takze klientow testu biegnacego rownoczesnie w innej przestrzeni — i pokazuje ich jako
# WLASNY wyciek. Pod `ctest -j 24` wywracalo to test na cudzych kolejkach.
QUEUE_GLOB="/dev/shm/brcdbr${RDB_NAMESPACE:+.$RDB_NAMESPACE}"*
QUEUES_BEFORE=$(ls $QUEUE_GLOB 2>/dev/null | wc -l)

xretractor "$1" -c

# Bez budzetu slotow (--llimitqry): dlugosc zycia serwera wyznacza `xqry -k` na koncu,
# a nie zgadniety limit iteracji. Poprzednie `-m 100` liczylo SLOTY, czyli czas scienny
# planu (~1,7 s dla query-lnx.rql), podczas gdy sam `xqry -s str2 -m 3` potrzebuje 1,5 s,
# bo str2 ma takt 1/2 s. Margines wynosil kilkadziesiat milisekund: kazde opoznienie na
# obciazonym CI wyczerpywalo budzet, serwer konczyl sie sam i kolejne polecenie klienta
# meldowalo "IPC: No such file or directory" albo "server not found". Odtworzone lokalnie
# jednym `sleep 0.5` wstawionym przed zapytania. Pozostale dziewiec testow serwerowych
# startuje wlasnie bez -m; higiena po nieudanym przebiegu nalezy do trap-u w serverlib.sh.
server_start "$1" -k -x

# Blokada dowodzi, ze serwer wstal; gotowosc kanalu IPC sprawdzamy osobno.
READY=0
for _ in $(seq 1 10); do
  if xqry -d > /dev/null 2>&1; then
    READY=1
    break
  fi
  sleep 0.5
done
if [ "$READY" -ne 1 ]; then
  echo "xretractor nie przyjmuje zapytan IPC po 5 s"
  exit 1
fi

xqry -d
xqry -s "$2" -m 3
xqry -s "$3" -m 3
xqry -l

xqry -k || true
server_wait_exit

QUEUES_AFTER=$(ls $QUEUE_GLOB 2>/dev/null | wc -l)
if [ "$QUEUES_AFTER" -gt "$QUEUES_BEFORE" ]; then
  echo "LEAK: brcdbr queue count increased from $QUEUES_BEFORE to $QUEUES_AFTER"
  ls $QUEUE_GLOB 2>/dev/null
  exit 1
fi
