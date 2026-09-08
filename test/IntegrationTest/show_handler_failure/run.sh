#!/bin/bash
# Awaria handlera 'show' musi dojechac do klienta jako POWOD, a nie jako cisza.
#
# Pulapka, ktora ten test zamyka (CI 2026-09-04, it_fncall_runtime_case). Handler
# 'show' nie wpisuje niczego do odpowiedzi TAKZE wtedy, gdy subskrypcja sie uda, a
# catch-all w commandProcessor polykal wyjatek i mimo to odsylal odpowiedz nie do
# odroznienia od powodzenia. Klient ruszal z watkiem producenta i meldowal dopiero
# "response queue did not appear" -- objaw oddalony o krok od przyczyny, ktorej nazwa
# zostawala w logu serwera w TMPDIR i przepadala razem z kontenerem CI.
#
# Wyscigu z CI nie da sie zamowic, wiec awarie wymusza hak RDB_FAULT_SHOW czytany
# przez handler. Test sprawdza dwie rzeczy naraz: ze klient konczy sie wlasciwym
# kodem i ze w jego logu jest POWOD podany przez serwer.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp

# Hak czyta wylacznie serwer; klient dostaje juz srodowisko bez niego, zeby bylo
# widac, ze powod przyszedl po linii IPC, a nie ze zmiennej.
export RDB_FAULT_SHOW=1
server_start query.rql -k -x
unset RDB_FAULT_SHOW

# Log klienta jest WSPOLNY dla przestrzeni nazw i dopisywany, a nie nadpisywany, wiec
# bez oproznienia negatywne sprawdzenie ponizej lapaloby zdanie sasiada z tego samego
# slotu puli. Komunikaty xqry ida do pliku w TMPDIR (patrz it_issue217_client_diag_stderr).
QRY_LOG="${TMPDIR:-/tmp}/xqry.log"
: > "$QRY_LOG"

rc=0
xqry -s dst -m 2 > out.txt 2> err.txt || rc=$?

# 63 = no_stream_resources, czyli selectResult::clientQueueMissing. Kolejki faktycznie
# nie ma, wiec werdykt jest ten sam co przed naprawa -- zmienia sie moment i uzasadnienie.
if [ "$rc" -ne 63 ]; then
  echo "xqry zakonczyl sie kodem $rc, oczekiwano 63"
  cat out.txt err.txt
  exit 1
fi

# Sedno regresji: powod z serwera, a nie sam brak kolejki.
if ! grep -F "RDB_FAULT_SHOW" "$QRY_LOG"; then
  echo "log klienta nie nazywa przyczyny odmowy; tresc $QRY_LOG:"
  cat "$QRY_LOG"
  exit 1
fi

# Klient nie ma prawa czekac na kolejke, ktorej serwer swiadomie nie utworzyl.
if grep -F "did not appear after" "$QRY_LOG"; then
  echo "klient nadal czekal na kolejke zamiast odczytac odmowe z odpowiedzi na 'show'"
  exit 1
fi

xqry -k > /dev/null
server_wait_exit
