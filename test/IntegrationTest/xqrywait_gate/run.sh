#!/bin/bash
# Bramka --xqrywait (-x) wstrzymuje przetwarzanie do pierwszej komendy klienta.
# Ten test pilnuje dwoch niezaleznych wlasnosci tej bramki.
#
# Pulapka 1 -- zgubiona pobudka. Watek komunikacyjny podnosil bramke tylko wtedy, gdy
# widzial juz flage oczekiwania, a watek glowny ustawial ja DOPIERO po zbudowaniu
# dataModel. Blokada instancji ("PID: <pid>", kontrakt server_start) publikowana jest
# duzo wczesniej, wiec miedzy "serwer gotowy" a bramka byla dziura: zmierzona na ok.
# 90 ms dla planu o 120 strumieniach. Komenda z tej dziury przepadala i serwer stal na
# bramce az do NASTEPNEJ komendy. Odtworzone 5/5 przed naprawa.
#
# Pulapka 2 -- skasowany budzet slotow. Flaga bramki byla trzymana w tym samym liczniku,
# co budzet z --llimitqry, a podniesienie bramki wpisywalo do niego "bez ograniczenia".
# `xretractor -m 5` konczyl sie sam, `xretractor -x -m 5` chodzil bez konca. To wlasnosc
# deterministyczna, bez zadnego wyscigu.
#
# Obserwabla jest w obu przypadkach ta sama i jednoznaczna: przy -m 5 serwer, ktoremu
# bramka zostala podniesiona, konczy sie SAM po budzecie. Zawieszona bramka albo
# skasowany budzet znacza proces, ktory zyje dalej. Nie parsujemy stdout serwera --
# przy przekierowaniu do pliku jest on buforowany blokowo i klamie.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp

# Plan szeroki: konstrukcja dataModel dla 120 strumieni to cale okno startowe. Limit
# gniazda magistrali to 128 strumieni, wiec 120 + zrodlo miesci sie na styk.
{
  echo "STORAGE 'temp'"
  echo
  echo "DECLARE a INTEGER STREAM src, 1/8 FILE 'data.txt'"
  echo
  for i in $(seq 0 119); do echo "SELECT src[0] STREAM dst$i FROM src"; done
} > wide.rql

{
  echo "STORAGE 'temp'"
  echo
  echo "DECLARE a INTEGER STREAM src, 1/8 FILE 'data.txt'"
  echo
  echo "SELECT src[0] STREAM dst FROM src"
} > small.rql

# Budzet 5 slotow po 1/8 s to ok. 0,6 s pracy; 15 s to zapas na obciazona maszyne CI.
wait_for_exit() {
  local pid="$1" left=150
  while [ "$left" -gt 0 ]; do
    kill -0 "$pid" 2>/dev/null || return 0
    sleep 0.1
    left=$((left - 1))
  done
  return 1
}

# --- Wlasnosc 2: budzet slotow przezywa bramke (deterministyczna) ---
# Komenda idzie PO oknie startowym, wiec zgubiona pobudka nie ma tu nic do rzeczy:
# sprawdzamy sam budzet.
server_start small.rql -k -x -m 5
sleep 1
xqry -l > /dev/null 2>&1
if ! wait_for_exit "$_server_pid"; then
  echo "budzet -m 5 przepadl przez bramke -x: serwer zyje mimo wyczerpanego budzetu"
  exit 1
fi
server_wait_exit

# --- Wlasnosc 1: komenda z okna startowego podnosi bramke ---
# Pojedyncza komenda ('hello' nie wymaga dataModel, wiec obsluguje sie w calosci wewnatrz
# okna) wysylana natychmiast po opublikowaniu blokady. Test jest wspomagany czasowo:
# moze bledowac na zielono, gdy okno zamknie sie szybciej niz wstanie klient, ale NIGDY
# na czerwono. Trzy powtorzenia, bo przed naprawa kazde konczylo sie zawieszeniem.
for attempt in 1 2 3; do
  rm -rf temp
  mkdir -p temp
  server_start wide.rql -k -x -m 5
  xqry -l > /dev/null 2>&1
  if ! wait_for_exit "$_server_pid"; then
    echo "proba $attempt: bramka -x nie zostala podniesiona przez komende z okna startowego"
    exit 1
  fi
  server_wait_exit
done
