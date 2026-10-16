#!/bin/bash
# Rezerwacja numeru licznika rotacji (#521, etap 9b). Kontrakt executorsm: blad odczytu LUB rezerwacji
# numeru licznika to Errc::IO i kod EIO. Odczyt sprawdza run.sh; tu licznik czyta sie poprawnie
# (kontrola wstepna przy starcie i reset-commit go przepuszczaja), ale zapisu nastepnego numeru
# odmawia katalog tylko do odczytu. Ta porazka wypada dopiero w executorsm: przy starcie w run(),
# po --reset w applyPendingPlan.
#
# Root pisze mimo chmod 555, wiec pod rootem (kontener CI) weryfikacja jest niewykonana: kod 77.
set -e
. "$(dirname "$0")/../serverlib.sh"

if [ "$(id -u)" -eq 0 ]; then
  echo "NIEWYKONANA WERYFIKACJA: root ignoruje prawa katalogu licznika"
  exit 77
fi

eio=$(errno_value EIO)
# Prawa katalogu wracaja przy kazdym wyjsciu, inaczej katalogu buildu nie da sie usunac.
# Kod wyjscia przechodzi do server_cleanup, ktory czyta go z $?.
trap 'rc=$?; chmod u+w rodir 2>/dev/null; (exit $rc); server_cleanup' EXIT INT TERM

chmod u+w rodir 2>/dev/null || true
rm -rf ./temp rodir counter.txt && mkdir -p ./temp rodir
printf '5' > rodir/counter.txt
chmod 555 rodir
printf '%s\n' "ROTATION 'rodir/counter.txt'" "STORAGE 'temp'" "DECLARE a INTEGER[3] STREAM src3, 1/8 TEXTFILE 'data.txt'" \
  "SELECT src3[0]+3 STREAM dst3 FROM src3" > rodir_plan.rql

# --- 1. Start: numer 6 nie daje sie zarezerwowac. ---
status=0
run_timeout 30 xretractor rodir_plan.rql -k -m 3 >/dev/null 2>start_reserve.err || status=$?
if [ "$status" -ne "$eio" ]; then
  echo "rezerwacja numeru przy starcie: kod $status, oczekiwano $eio (EIO)"
  cat start_reserve.err
  exit 1
fi
grep -q "Cannot reserve rotation number 6 in 'rodir/counter.txt'" start_reserve.err || {
  echo "rezerwacja numeru przy starcie: komunikat nie nazwal numeru i pliku"
  cat start_reserve.err
  exit 1
}
[ "$(cat rodir/counter.txt)" = "5" ] || { echo "odmowa zmienila plik licznika"; exit 1; }

# --- 2. Po --reset: plan przyjety, porazka przy wymianie epoki konczy proces kodem EIO. ---
server_start plan.rql -k 2>reset_reserve.err
xqry --reset rodir_plan.rql >/dev/null
server_wait_status > reset_reserve.status  # nie w $(...): podpowloka nie czeka na cudze dziecko
status=$(cat reset_reserve.status)
if [ "$status" -ne "$eio" ]; then
  echo "rezerwacja numeru po --reset: kod $status, oczekiwano $eio (EIO)"
  cat reset_reserve.err
  exit 1
fi
grep -q "FATAL (environment): Cannot reserve rotation number 6" reset_reserve.err || {
  echo "rezerwacja numeru po --reset: brak komunikatu z przedrostkiem klasy na stderr"
  cat reset_reserve.err
  exit 1
}
echo "OK"
