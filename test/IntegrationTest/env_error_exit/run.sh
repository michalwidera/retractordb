#!/bin/bash
# Klasy bledu srodowiska i licznika rotacji (#472).
#
# Licznik rotacji dzieli porazki wedlug PRZYCZYNY, bo usluga reaguje na nie inaczej:
#   1. tresc, ktora nie jest nieujemna liczba, to wadliwy stan wejsciowy planu (K2): odmowa
#      planu kodem EPROTO - w jednostce systemd plan znika z pliku zapytan i usluga wstaje
#      bezczynna, zamiast wracac co RestartSec na ten sam plan;
#   2. pliku nie da sie odczytac (tu: katalog w miejscu pliku - chmod 000 nie zatrzymuje roota
#      w kontenerze CI) - to srodowisko (K4): kod EIO, plan zachowany, usluga wraca sama po
#      naprawie;
#   3. przy przeladowaniu (`xqry --reset`) obie przyczyny sa odmowa zadania, a dotychczasowy
#      plan liczy dalej - sprawdzenie stoi przed rezerwacja na magistrali i rozbiorem planu.
#
#   4. Blad srodowiska w SLOCIE przetwarzania konczy proces kodem EIO, przedrostkiem
#      "FATAL (environment):" i bez zawieszenia, ze zwolniona blokada i posprzatanym IPC.
#      Zadne RQL nie wywoluje awarii nosnika na zawolanie, stad hak RDB_FAULT_ENV_IN_SLOT.
#      Zachowania pliku zapytan uslugi ten test nie widzi (nie biegnie w jednostce systemd);
#      pilnuje go zatrzask `latch=0` w ut_fatalError i warunek w executorsm::cleanup().
set -e
. "$(dirname "$0")/../serverlib.sh"

# Kody z nazw stalych, nie z liczb: EPROTO to 71 na Linuksie i 100 na macOS.
eproto=$(errno_value EPROTO)
eio=$(errno_value EIO)

reset_tree() {
  rm -rf ./temp counter.txt other_counter.txt && mkdir -p ./temp
}

# --- 1. Wadliwa tresc licznika przy starcie: odmowa planu, licznik nietkniety. ---
reset_tree
printf 'garbage' > counter.txt
status=0
run_timeout 30 xretractor plan.rql -k -m 3 >/dev/null 2>start_invalid.err || status=$?
if [ "$status" -ne "$eproto" ]; then
  echo "wadliwy licznik przy starcie: kod $status, oczekiwano $eproto (EPROTO)"
  cat start_invalid.err
  exit 1
fi
grep -q "rotation counter file .*counter.txt' is unreadable" start_invalid.err || {
  echo "wadliwy licznik przy starcie: odmowa nie nazwala pliku licznika"
  cat start_invalid.err
  exit 1
}
[ "$(cat counter.txt)" = "garbage" ] || { echo "odmowa zmienila plik licznika"; exit 1; }

# --- 2. Licznik nieczytelny przy starcie: EIO, nie odmowa planu. ---
reset_tree
mkdir counter.txt
status=0
run_timeout 30 xretractor plan.rql -k -m 3 >/dev/null 2>start_unreadable.err || status=$?
if [ "$status" -ne "$eio" ]; then
  echo "nieczytelny licznik przy starcie: kod $status, oczekiwano $eio (EIO)"
  cat start_unreadable.err
  exit 1
fi
grep -q "rotation counter file .*counter.txt' cannot be read" start_unreadable.err || {
  echo "nieczytelny licznik przy starcie: komunikat nie nazwal przyczyny"
  cat start_unreadable.err
  exit 1
}

# --- 3. Wadliwy i nieczytelny licznik kandydata przy resecie: odmowa, stary plan liczy dalej. ---
reset_tree
server_start plan.rql -k
for kind in invalid unreadable; do
  rm -rf other_counter.txt
  if [ "$kind" = invalid ]; then printf -- '-1' > other_counter.txt; else mkdir other_counter.txt; fi
  status=0
  xqry --reset other.rql > reset_$kind.out 2> reset_$kind.err || status=$?
  if [ "$status" -eq 0 ]; then
    echo "reset z licznikiem ($kind) zostal przyjety"
    cat reset_$kind.out reset_$kind.err
    exit 1
  fi
  grep -q "rotation counter file" reset_$kind.out reset_$kind.err || {
    echo "odmowa resetu ($kind) nie nazwala licznika"
    cat reset_$kind.out reset_$kind.err
    exit 1
  }
  if ! run_timeout 30 xqry -s dst -m 2 > after_$kind.txt 2>&1; then
    echo "po odrzuconym resecie ($kind) dotychczasowy plan nie odpowiada"
    cat after_$kind.txt
    exit 1
  fi
done
xqry -k >/dev/null 2>&1 || true
server_wait_exit

# --- 4. Blad srodowiska w slocie: EIO, przedrostek klasy, skonczony czas, blokada zwolniona. ---
reset_tree
status=0
RDB_FAULT_ENV_IN_SLOT=1 run_timeout 30 xretractor plan.rql -k >/dev/null 2>slot.err || status=$?
if [ "$status" -ne "$eio" ]; then
  echo "blad srodowiska w slocie: kod $status, oczekiwano $eio (EIO; 124 = zawieszenie)"
  cat slot.err
  exit 1
fi
grep -q "FATAL (environment): fault hook RDB_FAULT_ENV_IN_SLOT" slot.err || {
  echo "blad srodowiska w slocie: brak komunikatu z przedrostkiem klasy na stderr"
  cat slot.err
  exit 1
}
if [ -e "$SERVER_LOCK" ] && ! lock_is_free "$SERVER_LOCK"; then
  echo "blad srodowiska w slocie: blokada instancji zostala zajeta po wyjsciu"
  exit 1
fi
rm -f "$SERVER_LOCK"

echo "OK"
