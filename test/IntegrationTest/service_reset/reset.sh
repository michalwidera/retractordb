#!/bin/bash
# Przeladowanie calego planu dzialajacej uslugi jedna komenda: `xqry --reset plan.rql`.
#
# Scenariusz sprawdza cztery wlasciwosci, ktorych zaden istniejacy test nie obejmowal:
#
#  1. Instancja BEZCZYNNA (start bez pliku planu) potrafi przyjac plan. Do 2026-09-05 byla
#     slepym zaulkiem: pProc pozostawal nullem, wiec kazda komenda wymagajaca modelu
#     przechodzila przez wszystkie `if`-y i wracala PUSTA odpowiedzia, a jedyna droga do
#     zaladowania planu byl restart procesu.
#  2. Plan wadliwy jest odrzucany PRZED dotknieciem planu dzialajacego. To jest cala roznica
#     miedzy przeladowaniem a restartem: odmowa nie moze kosztowac uslugi.
#  3. Podmiana planu zwalnia nazwy strumieni poprzednika na magistrali i rosci nowe.
#  4. Usluga jest DOKLADNIE JEDNA. Serwerow zwyklych moze pracowac wiele, ale druga instancja
#     w trybie uslugowym odpowiadalaby na te same komendy i pisala do tego samego pliku
#     zapytan.
#
# Katalog dziala na tozsamosci globalnej maszyny (nazwa instancji `service`), stad
# IT_NO_NAMESPACE w CMakeLists.txt i RUN_SERIAL.
set -e
. "$(dirname "$0")/../serverlib.sh"

# Usluga ma stala nazwe instancji, wiec i stala nazwe pliku blokady. Oprawa domysla sie
# nazwy z RDB_NAMESPACE, a ten katalog przestrzeni nazw nie uzywa.
SERVER_LOCK="${TMPDIR:-/tmp}/xretractor_service.service.lock"

rm -rf ./temp && mkdir -p ./temp

# Wiersz magistrali dla instancji `service`. Pusta kolumna STREAMS to znak '-'.
service_row() { xqry --bus 2>/dev/null | grep -E '^service +\|' || true; }

# Czeka, az plan uslugi zacznie roscic dany strumien. Przeladowanie jest asynchroniczne:
# `--reset` konczy sie na PRZYJECIU planu, a wymiana epoki nastepuje w watku przetwarzania.
wait_for_stream() {
  local name="$1" i=0
  while [ "$i" -lt 100 ]; do
    if service_row | grep -qE "[ |,]${name}(,|\$| )"; then return 0; fi
    sleep 0.1
    i=$((i + 1))
  done
  echo "strumien '${name}' nie pojawil sie w planie uslugi w ciagu 10 s"
  xqry --bus || true
  return 1
}

# --- 1. Start bezczynny: instancja jest na magistrali, ale nie liczy niczego. ---
server_start --service --noanykey

if ! service_row | grep -qE '\| S +\|'; then
  echo "instancja 'service' nie zglosila trybu uslugowego na magistrali"
  xqry --bus || true
  exit 1
fi

# Instancja bez planu ODPOWIADA, a nie milczy. Do 2026-09-05 komenda wymagajaca modelu
# wracala z pustym ptree: klient meldowal wtedy "server did not answer within the timeout",
# czyli obciazal serwer awaria, ktorej nie bylo, a `xqry -d` wywracalo sie wyjatkiem
# o brakujacym wezle ptree. Diagnoza szla wtedy na IPC zamiast na brak planu.
status=0
xqry -s alpha -m 2 --server service > idle_select.txt 2> idle_select_err.txt || status=$?
if [ "$status" -eq 0 ]; then
  echo "odczyt strumienia z instancji bez planu zakonczyl sie sukcesem"
  exit 1
fi
grep -q 'no plan loaded' idle_select_err.txt || {
  echo "klient nie rozpoznal instancji bez planu"
  cat idle_select_err.txt
  exit 1
}
xqry -d --server service > idle_dir.txt
grep -q 'no active plan' idle_dir.txt || {
  echo "'xqry -d' nie nazwal stanu instancji bez planu"
  cat idle_dir.txt
  exit 1
}

# --- 2. Pierwszy plan wchodzi do dzialajacej instancji. ---
xqry --reset plan1.rql --server service
wait_for_stream alpha
xqry -s alpha -m 3 --server service > out_alpha.txt
if [ "$(wc -l < out_alpha.txt)" -lt 3 ]; then
  echo "strumien alpha nie oddal 3 rekordow po zaladowaniu planu"
  cat out_alpha.txt
  exit 1
fi

# --- 3. Plan wadliwy: odmowa, a dzialajacy plan zostaje nietkniety. ---
status=0
xqry --reset bad.rql --server service > bad_out.txt 2> bad_err.txt || status=$?
if [ "$status" -eq 0 ]; then
  echo "wadliwy plan zostal przyjety"
  cat bad_out.txt bad_err.txt
  exit 1
fi
grep -q 'plan reload refused' bad_err.txt || {
  echo "odmowa nie nazwala powodu na stderr"
  cat bad_err.txt
  exit 1
}
wait_for_stream alpha
xqry -s alpha -m 2 --server service > out_alpha_after_bad.txt
if [ "$(wc -l < out_alpha_after_bad.txt)" -lt 2 ]; then
  echo "po odrzuconym planie strumien alpha przestal liczyc"
  cat out_alpha_after_bad.txt
  exit 1
fi

# --- 4. Podmiana planu: nowe nazwy wchodza, stare znikaja z magistrali. ---
xqry --reset plan2.rql --server service
wait_for_stream beta
if service_row | grep -qE '[ |,]alpha(,|$| )'; then
  echo "po podmianie planu stary strumien alpha jest nadal roszczony"
  xqry --bus || true
  exit 1
fi
xqry -s beta -m 3 --server service > out_beta.txt
if [ "$(wc -l < out_beta.txt)" -lt 3 ]; then
  echo "strumien beta nie oddal 3 rekordow po podmianie planu"
  cat out_beta.txt
  exit 1
fi

# --- 5. Plan pusty sprowadza usluge z powrotem do stanu zerowego. ---
xqry --reset empty.rql --server service
i=0
while [ "$i" -lt 100 ]; do
  if ! service_row | grep -qE '[ |,]beta(,|$| )'; then break; fi
  sleep 0.1
  i=$((i + 1))
done
if service_row | grep -qE '[ |,]beta(,|$| )'; then
  echo "pusty plan nie zwolnil nazw strumieni"
  xqry --bus || true
  exit 1
fi

# --- 6. Druga usluga jest odmawiana; zwykly serwer w tym czasie wstaje bez przeszkod. ---
status=0
xretractor --service --noanykey --name second </dev/null > second.txt 2>&1 || status=$?
if [ "$status" -eq 0 ]; then
  echo "druga instancja uslugowa wystartowala"
  cat second.txt
  exit 1
fi
grep -q 'only one service instance is allowed' second.txt || {
  echo "odmowa drugiej uslugi nie nazwala powodu"
  cat second.txt
  exit 1
}

xretractor plan2.rql --noanykey --name plain -m 6 </dev/null > plain.txt 2>&1 || {
  echo "zwykly serwer nie wystartowal obok uslugi"
  cat plain.txt
  exit 1
}

# --- 7. Zatrzymanie uslugi. ---
xqry -k --server service
server_wait_exit
