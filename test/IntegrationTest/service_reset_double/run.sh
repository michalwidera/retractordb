#!/bin/bash
# Dwa resety zlozone w czasie nie moga pomieszac planu z rezerwacja magistrali.
#
# Defekt, ktory ten test zamyka (odtworzony 2026-09-06). Gniazdo magistrali trzyma
# DOKLADNIE JEDNA rezerwacje planu: reservePlan() ja nadpisuje, activateReservedPlan()
# zuzywa. resetCommit() rezerwowal zasoby juz przy PRZYJECIU zestawu, a wymiana zdejmowala
# znacznik zadania (planResetRequested) na samym POCZATKU applyPendingPlan() — czyli okolo
# szescdziesieciu linii przed aktywacja. W oknie miedzy zabraniem tekstu planu a jego
# aktywacja kolejny reset nadpisywal rezerwacje planu wlasnie wchodzacego:
#
#   17:24:57.684 [I] Plan reload accepted; the running plan will be replaced.
#   17:24:57.694 [I] Plan reloaded: 102 node(s) in the new plan.
#   17:24:57.757 [C] Cannot activate the reserved bus resources: instance holds no plan reservation
#
# Odchodzacy plan aktywowal cudza rezerwacje (oglaszajac na magistrali nazwy, ktorych nie
# liczy), a nastepna epoka nie miala juz czego aktywowac i konczyla sie FatalError. W trybie
# --service ten sam blad czysci plik zapytan, wiec jednostka wstawala BEZ planu: dwoch
# operatorow wolajacych `xqry --reset` jednoczesnie kasowalo plan produkcyjny.
#
# Naprawa: znacznik planSwapInFlight zyje od przyjecia zestawu do aktywacji jego rezerwacji,
# a resetCommit() odrzuca kazdy zestaw przyslany w tym czasie.
#
# UKLAD DOSWIADCZENIA. Trafienie w okno przez sam wyscig kosztowalo omiatanie przesuniecia
# miedzy dwoma klientami (jedno trafienie na czternascie prob), wiec okno otwiera hak
# RDB_FAULT_PLAN_SWAP_DELAY — ta sama droga co RDB_FAULT_GET_AWAIT_EPOCH_SWAP w
# it_service_reset_race. Hak wstrzymuje wymiane dokladnie tam, gdzie tekst planu jest juz
# zabrany, a rezerwacja jeszcze nie aktywowana.
#
# Test sprawdza CZTERY rzeczy, bo naprawa ma dwie strony i obie da sie zepsuc osobno:
#   1. reset przyslany, zanim wymiana ruszy, jest odrzucony (zestaw przyjety i nadpisany
#      przez nastepny nigdy nie ruszal, a jego klient dostawal "OK");
#   2. reset przyslany W OKNIE wymiany jest odrzucony — to jest sedno regresji;
#   3. serwer zyje, a plan, ktory zostal przyjety, faktycznie wszedl;
#   4. reset przyslany PO wymianie jest przyjety. Bez tego naprawa "odrzucaj zawsze"
#      przechodzilaby trzy pierwsze punkty i unieruchamiala przeladowanie planu na stale.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf ./temp && mkdir -p ./temp

# Segment magistrali nalezy do przestrzeni nazw tego testu, wiec jedyne wiersze, jakie w nim
# sa, pochodza od naszej instancji. Gotowosc planu sprawdzamy wlasnie tedy, a nie komenda
# do serwera: `xqry -d` czeka na model nowej epoki, wiec mieszalby przyrzad z przedmiotem.
wait_for_stream() {
  local name="$1" i=0
  while [ "$i" -lt 150 ]; do
    if xqry --bus 2>/dev/null | grep -qE "[ |,]${name}(,| |\||$)"; then return 0; fi
    sleep 0.1
    i=$((i + 1))
  done
  echo "strumien '${name}' nie pojawil sie na magistrali w ciagu 15 s"
  xqry --bus || true
  return 1
}

# Odmowa konczy `xqry` kodem niezerowym, a skrypt biegnie pod `set -e`.
reset_plan() {
  local plan="$1" out="$2"
  xqry --reset "$plan" > "$out" 2>&1 || true
}

# Okno dluzsze niz takt planu (1 s), zeby drugi reset zdazyl w nie trafic z zapasem, i
# krotsze niz limit czasu testu.
export RDB_FAULT_PLAN_SWAP_DELAY=4000
server_start plan1.rql --noanykey
unset RDB_FAULT_PLAN_SWAP_DELAY

wait_for_stream alpha1

# --- 1. Reset przyjety; drugi, przyslany zanim wymiana ruszy, ma odpasc ---
reset_plan plan2.rql accepted.txt
# Warunek przez `if`, a nie `grep ... && { ... }`: pod `set -e` AND-lista zakonczona
# nietrafionym grepem konczy caly skrypt, wiec przypadek ZIELONY wywracalby test.
if grep -q "refused" accepted.txt; then
  echo "pierwszy reset zostal odrzucony, choc zadna wymiana nie trwala:"
  cat accepted.txt
  exit 1
fi

reset_plan plan3.rql refused_early.txt
grep -q "a plan reload is already in progress" refused_early.txt || {
  echo "reset przyslany na przyjety, jeszcze niezastosowany plan nie zostal odrzucony:"
  cat refused_early.txt
  exit 1
}

# --- 2. Reset przyslany W OKNIE wymiany ---
# Wymiana zaczyna sie na najblizszej granicy slotu (takt 1 s), a hak trzyma ja przez 4 s.
# Dwie sekundy po przyjeciu jestesmy wiec w srodku okna, z zapasem po obu stronach.
sleep 2
reset_plan plan3.rql refused_inside.txt
grep -q "a plan reload is already in progress" refused_inside.txt || {
  echo "reset przyslany w trakcie wymiany planu nie zostal odrzucony:"
  cat refused_inside.txt
  exit 1
}

# --- 3. Serwer zyje, a przyjeta wymiana doszla do skutku ---
wait_for_stream beta1

if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer nie przezyl wymiany planu"
  exit 1
fi

# Plan odrzucony nie ma prawa nic po sobie zostawic na magistrali.
if xqry --bus 2>/dev/null | grep -qE "[ |,]gamma1(,| |\||$)"; then
  echo "na magistrali sa strumienie planu, ktory zostal odrzucony:"
  xqry --bus
  exit 1
fi

# --- 4. Po zakonczonej wymianie reset znowu jest przyjmowany ---
reset_plan plan3.rql accepted_after.txt
if grep -q "refused" accepted_after.txt; then
  echo "po zakonczonej wymianie reset nadal jest odrzucany — znacznik zostal podniesiony:"
  cat accepted_after.txt
  exit 1
fi
wait_for_stream gamma1

xqry -k > /dev/null 2>&1
server_wait_exit
