#!/bin/bash
# #283: subscribe() buduje kolejke odpowiedzi poza blokada epoki planu. Miedzy odczytem
# parametrow strumienia a rejestracja epoka moze sie wiec skonczyc, a rejestracja spozniona
# wzgledem OOB przezylaby ja: przy --reset klient dostawalby bez ostrzezenia wiersze strumienia
# NOWEGO planu o tej samej nazwie, ze schematem odczytanym ze starego. Serwer ma takiej
# subskrypcji odmowic i podac powod.
#
# Kolejnosc jest wymuszona, nie zgadywana:
#  - plan odchodzacy czeka w kazdym slocie 3 s na DEVICE (FIFO z pisarzem, bez danych), poza
#    blokada epoki, wiec --reset przyjety teraz rozbiera epoke dopiero po tym czekaniu;
#  - `show` wysylany zaraz po --reset zastaje stary model i odczytuje z niego parametry;
#  - hak RDB_FAULT_SUBSCRIBE_AWAIT_PLAN_SWAP trzyma handler miedzy zwolnieniem blokady a
#    subscribe(), az rejestr subskrypcji zostanie zamkniety (OOB starej epoki) i nowy plan
#    ogloszony. Bez drugiego warunku nastepna komenda klienta (`get`) trafialaby w okno bez
#    planu i konczyla sie werdyktem "no active plan" zamiast odczytu odmowy.
#
# Hak czeka w handlerze do 3 s (TIMEOUT DEVICE) plus czas wymiany planu, a ponowna subskrypcja
# na koncu wyczerpuje caly jego budzet (6 s), bo epoka sie juz nie zmienia. Domyslny limit
# odpowiedzi klienta to 3 s, wiec xqry dostaje 10 s przez client.toml.
#
# Test nie przechodzi falszywie. Gdyby `show` nie trafil w stara epoke, hak wyczerpalby budzet
# bez zamkniecia rejestru, subskrypcja by sie udala i klient dostalby wiersze -- czerwono.
# Bez naprawy (rejestracja bez sprawdzenia epoki) klient dostaje wiersze nowego planu i konczy
# sie zerem -- czerwono.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf old new wait.fifo
mkdir -p old new
mkfifo wait.fifo
exec 3<>wait.fifo
printf '[ipc]\nclient_response_max_fails = 1000\n' > client.toml

export RDB_FAULT_SUBSCRIBE_AWAIT_PLAN_SWAP=6000
server_start old.rql -k
unset RDB_FAULT_SUBSCRIBE_AWAIT_PLAN_SWAP

# Log klienta jest wspolny dla przestrzeni nazw i dopisywany (patrz it_show_handler_failure).
QRY_LOG="${TMPDIR:-/tmp}/xqry.log"
: > "$QRY_LOG"

xqry --reset new.rql
rc=0
run_timeout 60 xqry -s dst -m 2 --config client.toml > out.txt 2> err.txt || rc=$?

# no_stream_resources = selectResult::clientQueueMissing: strumien istnieje (w nowym planie),
# ale kolejki odpowiedzi nie ma, bo serwer odmowil subskrypcji.
expected_rc=$(errno_value ENOSR)
if [ "$rc" -ne "$expected_rc" ]; then
  echo "xqry zakonczyl sie kodem $rc, oczekiwano $expected_rc"
  echo "--- out.txt"; cat out.txt
  echo "--- err.txt"; cat err.txt
  echo "--- $QRY_LOG"; cat "$QRY_LOG"
  exit 1
fi

if ! grep -F "plan epoch ended while subscribing to stream 'dst'" "$QRY_LOG"; then
  echo "log klienta nie nazywa przyczyny odmowy; tresc $QRY_LOG:"
  cat "$QRY_LOG"
  exit 1
fi

# Odmowa nie zostawia kolejki: po odmowie nie wolno widziec wierszy nowego planu.
if [ -s out.txt ]; then
  echo "klient wypisal wiersze mimo odmowy:"
  cat out.txt
  exit 1
fi

# Kontrola, ze serwer po odmowie obsluguje nowy plan normalnie: ponowna subskrypcja przechodzi.
run_timeout 60 xqry -s dst -m 2 --config client.toml > again.txt
[ "$(wc -l < again.txt)" -ge 2 ] || { echo "ponowna subskrypcja nie dala wierszy:"; cat again.txt; exit 1; }

xqry -k > /dev/null
server_wait_exit
exec 3>&-
