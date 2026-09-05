#!/bin/bash
# Wymiana planu nie ma prawa zabic serwera obslugujacego wlasnie klienta.
#
# Defekt, ktory ten test zamyka (odtworzony 2026-09-05). commandProcessor zwalnial
# core_mutex zaraz po przebudzeniu i dopiero POTEM siegal po model, czytajac globalny
# pProc na nowo przy kazdym uzyciu. Watek przetwarzania gasil ten wskaznik i rozbieral
# dataModel na koniec epoki, wiec handler juz w locie dostawal nulla albo obiekt
# zniszczony:
#
#   #4 dataModel::streamStoredSize (this=0x0, instance="beta60")
#   #5 executorsm::collectStreamsParameters ()
#   #6 executorsm::commandProcessor (...)          <- watek komunikacyjny
#   Thread 2: posixBinaryFileWithShadow::~posixBinaryFileWithShadow  <- rozbiorka epoki
#
# Naprawa: plan_epoch_mutex trzyma epoke przez caly czas obslugi komendy, a wymiana
# epoki czeka na jego zwolnienie.
#
# UKLAD DOSWIADCZENIA. Handler `get` dostaje hak RDB_FAULT_GET_AWAIT_EPOCH_SWAP: POMIEDZY
# sprawdzeniem pProc a pierwszym uzyciem modelu czeka (do wyczerpania budzetu w ms), az
# pProc zgasnie. Runda wyglada tak:
#
#   1. `--reset` konczy sie na PRZYJECIU planu (zmierzone: 45 ms). Rozbiorke wykonuje watek
#      przetwarzania dopiero na najblizszej granicy slotu, czyli w ciagu 1 s (takt planu).
#   2. `xqry -d` wystartowane zaraz potem wchodzi do handlera po ~50 ms i tam zostaje.
#
# Runda ma trzy mozliwe wyniki i tylko jeden z nich cos rozstrzyga:
#
#   * KOMENDA OBSLUZONA NA ODCHODZACEJ EPOCE -- trafienie. Silnik z naprawa konczy tak
#     kazda trafiona runde: wymiana czeka na handler, a nie handler na wymiane.
#   * "no active plan" -- pudlo. Rozbiorka wyprzedzila komende, ktora odbila sie od
#     sprawdzenia pProc i modelu nie dotknela. Nie dowodzi niczego, wiec runda idzie
#     od nowa; nie jest to blad ani silnika, ani testu.
#   * SMIERC SERWERA -- regresja. Tak konczy sie trafiona runda bez naprawy.
#
# Test wymaga TRZECH trafien, zeby zielony wynik nie mogl pochodzic z samych pudel.
# Falszywej czerwieni dac nie moze: pudlo jest powtarzane, a nie zliczane jako porazka.
#
# Odrzucone wersje, obie przechodzily TAKZE na silniku bez naprawy:
#   * czterech czytelnikow w wolnej petli, bez kontroli fazy -- watek komunikacyjny
#     obsluguje komendy pojedynczo, wiec o trafieniu w okno decyduje kolejnosc, a nie
#     liczba klientow;
#   * staly sen 2 s w handlerze -- okno, w ktorym pProc jest juz zgaszony, a nowa epoka
#     jeszcze nie opublikowana, trwa kilkanascie ms, wiec handler budzil sie PO wymianie
#     i konczyl poprawnie na nowym modelu.
#
# Gotowosc nowego planu sprawdzana jest przez `--bus`, a NIE przez `xqry -d`: hak dotyczy
# kazdej komendy `get`, wiec odpytywanie nim mieszaloby przyrzad z przedmiotem pomiaru.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf ./temp && mkdir -p ./temp

# Budzet dluzszy niz zmierzone ~1 s do rozbiorki epoki i krotszy niz timeout klienta
# (3 s = 300 x 10 ms), zeby komenda zdazyla dostac odpowiedz takze wtedy, gdy naprawa
# zatrzyma na niej wymiane planu na caly ten budzet.
export RDB_FAULT_GET_AWAIT_EPOCH_SWAP=1200
server_start plan1.rql --service --noanykey
unset RDB_FAULT_GET_AWAIT_EPOCH_SWAP

# Segment magistrali nalezy do przestrzeni nazw tego testu, wiec jedyne wiersze, jakie
# w nim sa, pochodza od naszej instancji.
wait_for_stream() {
  local name="$1" i=0
  while [ "$i" -lt 100 ]; do
    if xqry --bus 2>/dev/null | grep -qE "[ |,]${name}(,| |\||$)"; then return 0; fi
    sleep 0.1
    i=$((i + 1))
  done
  echo "strumien '${name}' nie pojawil sie na magistrali w ciagu 10 s"
  xqry --bus || true
  return 1
}

wait_for_stream alpha1

# Serwer ma byc w petli slotow, a nie w rozruchu: reset przyjety, zanim petla ruszy,
# jest widziany od razu na pierwszym sprawdzeniu warunku i rozbiorka wypada natychmiast,
# czyli przed komenda. To bylo zrodlo pudel w kazdej pierwszej rundzie.
sleep 2

live=alpha1
hits=0
attempt=0
while [ "$hits" -lt 3 ] && [ "$attempt" -lt 12 ]; do
  attempt=$((attempt + 1))
  if [ "$live" = alpha1 ]; then
    plan=plan2.rql
    want=beta1
  else
    plan=plan1.rql
    want=alpha1
  fi

  xqry --reset "$plan"
  # Komenda wchodzi do handlera, zanim watek przetwarzania zdazy rozebrac epoke.
  xqry -d > "dir_${attempt}.txt" 2> "dir_${attempt}_err.txt" &
  reader_pid=$!
  wait "$reader_pid" || {
    echo "proba ${attempt}: klient nie doczekal sie odpowiedzi w trakcie wymiany planu"
    cat "dir_${attempt}_err.txt"
    exit 1
  }

  if grep -q 'no active plan' "dir_${attempt}.txt"; then
    echo "proba ${attempt}: pudlo (rozbiorka wyprzedzila komende) — runda od nowa"
  else
    # Komenda obsluzona: musiala zostac obsluzona DO KONCA na modelu epoki odchodzacej.
    grep -qE "^${live} " "dir_${attempt}.txt" || {
      echo "proba ${attempt}: komenda obsluzona, ale nie na modelu odchodzacej epoki (${live})"
      cat "dir_${attempt}.txt"
      exit 1
    }
    hits=$((hits + 1))
  fi

  wait_for_stream "$want"
  live="$want"
done

if [ "$hits" -lt 3 ]; then
  echo "w ${attempt} probach udalo sie trafic w badane okno tylko ${hits} raz(y)"
  exit 1
fi

# Serwer przezyl wszystkie wymiany. To jest sedno regresji: bez naprawy trafiona runda
# konczy proces SIGSEGV-em, a kazde nastepne `xqry` odpada z bledu IPC.
if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer nie przezyl wymiany planu w trakcie obslugi klienta"
  exit 1
fi

# Wymiana zostala WYKONANA, a nie tylko przetrwana.
xqry -d > dir_final.txt
grep -qE "^${live} " dir_final.txt || {
  echo "po ostatniej wymianie plan nie oddaje strumienia ${live}"
  cat dir_final.txt
  exit 1
}

# Plan przekraczajacy pojemnosc slotu magistrali ma odpasc przed zakonczeniem
# biezacej epoki. Poprzednio walidacja sprawdzala tylko kolizje nazw, a limit 128
# wychodzil dopiero po rozebraniu modelu; klient dostawal sukces, a usluga zostawala
# bez aktywnego planu.
{
  echo "STORAGE 'temp'"
  echo "DECLARE a INTEGER STREAM oversized_src, 1 FILE 'data.txt'"
  i=1
  while [ "$i" -le 128 ]; do
    echo "SELECT a+${i} STREAM oversized${i} FROM oversized_src"
    i=$((i + 1))
  done
} > oversized.rql

status=0
xqry --reset oversized.rql > oversized_out.txt 2> oversized_err.txt || status=$?
if [ "$status" -eq 0 ]; then
  echo "plan ze 129 strumieniami zostal przyjety"
  cat oversized_out.txt oversized_err.txt
  exit 1
fi
grep -q 'plan reload refused' oversized_err.txt || {
  echo "odmowa zbyt duzego planu nie dotarla do klienta"
  cat oversized_err.txt
  exit 1
}
grep -q 'plan has 129 streams' oversized_err.txt || {
  echo "odmowa nie podala przekroczonego limitu magistrali"
  cat oversized_err.txt
  exit 1
}

# Odmowa zachowuje jednoczesnie model i jego roszczenie na magistrali.
xqry -d > dir_after_oversized.txt
grep -qE "^${live} " dir_after_oversized.txt || {
  echo "odrzucony zbyt duzy plan usunal dzialajacy plan (${live})"
  cat dir_after_oversized.txt
  exit 1
}
wait_for_stream "$live"
if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer nie przezyl odmowy zbyt duzego planu"
  exit 1
fi

xqry -k > /dev/null
server_wait_exit
