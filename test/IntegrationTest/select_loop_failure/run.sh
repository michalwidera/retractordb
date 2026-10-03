#!/bin/bash
# Awaria petli renderowania xqry musi dojechac do operatora jako POWOD, a nie jako
# pusty strumien (#285).
#
# Pulapka, ktora ten test zamyka. Cala petla renderowania qry::select byla owinieta
# w catch (...) logujacy staly napis "General exception catched.", po ktorym szedl
# zwykly epilog. Wyjatek formatera przy pierwszym elemencie konczyl sie wiec werdyktem
# noData ("delivered no elements") -- tym samym kodem i tym samym zdaniem co strumien,
# ktory naprawde nic nie przyslal -- a po czesci elementow sukcesem.
#
# Awarie wymusza hak RDB_FAULT_RENDER czytany przez KLIENTA (serwer go nie zna).
# Test sprawdza kod wyjscia, powod w logu klienta i brak mylacego werdyktu.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp

server_start query.rql -k -x

# Log klienta jest WSPOLNY dla przestrzeni nazw i dopisywany, wiec bez oproznienia
# negatywne sprawdzenie ponizej lapaloby zdanie sasiada z tego samego slotu puli.
QRY_LOG="${TMPDIR:-/tmp}/xqry.log"
: > "$QRY_LOG"

rc=0
RDB_FAULT_RENDER=1 xqry -s dst -m 2 > out.txt 2> err.txt || rc=$?

# selectResult::renderFailed -> boost::system::errc::interrupted, ten sam kod co wyjatek
# standardowy przechwycony na najwyzszym poziomie xqry. Liczba z nazwy stalej, jak
# w it_show_handler_failure.
expected_rc=$(errno_value EINTR)
if [ "$rc" -ne "$expected_rc" ]; then
  echo "xqry zakonczyl sie kodem $rc, oczekiwano $expected_rc"
  cat out.txt err.txt
  exit 1
fi

# Sedno regresji: miejsce i powod awarii, w logu i na stderr.
for f in "$QRY_LOG" err.txt; do
  if ! grep -F "select loop failed" "$f" | grep -F "RDB_FAULT_RENDER"; then
    echo "$f nie nazywa miejsca i przyczyny awarii; tresc:"
    cat "$f"
    exit 1
  fi
done

# Awaria nie ma prawa wygladac jak pusty strumien.
if grep -F "delivered no elements" "$QRY_LOG"; then
  echo "klient zameldowal pusty strumien zamiast awarii petli renderowania"
  exit 1
fi

xqry -k > /dev/null
server_wait_exit
