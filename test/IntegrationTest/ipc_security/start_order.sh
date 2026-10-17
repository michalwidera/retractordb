#!/bin/bash
# Kontrakt startu executorsm (#521, etap 9b). Linia "PID:" w pliku blokady jest dla klientow i dla
# server_start (serverlib.sh) sygnalem "serwer gotowy", wiec:
#
#  order       - zapis opisu procesu do pliku blokady wypada PO utworzeniu segmentu odpowiedzi
#                i kolejki komend. Kolejnosc czytamy ze sladu strace: wywolania systemowe wejscia
#                obu tworzacych openat() stoja w sladzie przed write() linii "PID:". Odwrocona
#                kolejnosc (publikacja przed czekaniem na gotowosc IPC) daje slad odwrotny przy
#                kazdym uruchomieniu, bez wyscigu.
#  ipc-failure - gdy zasobow IPC nie da sie zbudowac, start konczy sie kodem ENOBUFS
#                (no_buffer_space) z komunikatem, zamiast czekac bez konca. W przestrzeni nazw
#                uzytkownika z tmpfs 512 KiB na /dev/shm budowa odpada na kontroli katalogu IPC
#                (wlasciciel '/' spoza odwzorowania UID), a gdyby ta przepuscila - na braku miejsca.
#
# Brak strace albo nieuprzywilejowanych przestrzeni nazw (typowe w kontenerze CI) to weryfikacja
# niewykonana: kod 77, SKIP_RETURN_CODE w CMakeLists.txt.
set -euo pipefail
. "$(dirname "$0")/../portable.sh"

XRETRACTOR="$1"
MODE="$2"
NAME="${RDB_NAMESPACE:?test wymaga przestrzeni nazw}"
WORK="start_order.$MODE"

rm -rf "$WORK" && mkdir -p "$WORK"
cd "$WORK"
seq 1 20 > data.txt
printf '%s\n' "DECLARE a INTEGER STREAM src, 1/8 TEXTFILE 'data.txt'" "SELECT src[0] STREAM dst FROM src STORAGE MEMORY" \
  > query.rql

case "$MODE" in
  order)
    if ! command -v strace >/dev/null 2>&1 || ! strace -f -o /dev/null true 2>/dev/null; then
      echo "NIEWYKONANA WERYFIKACJA: strace niedostepny albo ptrace zabroniony"
      exit 77
    fi
    rc=0
    run_timeout 30 strace -f -e trace=openat,write -o trace.txt "$XRETRACTOR" query.rql -k -m 3 </dev/null \
      > out.txt 2> err.txt || rc=$?
    if [ "$rc" -ne 0 ]; then
      echo "serwer pod strace zakonczyl sie kodem $rc"
      cat err.txt
      exit 1
    fi
    line_of() { grep -nE "$1" trace.txt | head -1 | cut -d: -f1; }
    reply=$(line_of "openat\(.*\"/dev/shm/RetractorReply_v1\.${NAME}\", [^)]*O_CREAT")
    queue=$(line_of "openat\(.*\"/dev/shm/RetractorQueryQueue\.${NAME}\", [^)]*O_CREAT")
    pidline=$(line_of 'write\([0-9]+, "PID: [0-9]+')
    if [ -z "$reply" ] || [ -z "$queue" ] || [ -z "$pidline" ]; then
      echo "slad niekompletny: segment odpowiedzi='$reply', kolejka komend='$queue', zapis PID='$pidline'"
      grep -E "Retractor|PID: " trace.txt || true
      exit 1
    fi
    if [ "$pidline" -le "$reply" ] || [ "$pidline" -le "$queue" ]; then
      echo "linia PID opublikowana przed utworzeniem zasobow IPC (wiersze sladu: PID $pidline," \
        "segment $reply, kolejka $queue)"
      exit 1
    fi
    echo "OK: segment (wiersz $reply) i kolejka (wiersz $queue) przed zapisem PID (wiersz $pidline)"
    ;;
  ipc-failure)
    if ! command -v unshare >/dev/null 2>&1 ||
      ! unshare -Umr sh -c 'mount -t tmpfs -o size=512k tmpfs /dev/shm' 2>/dev/null; then
      echo "NIEWYKONANA WERYFIKACJA: brak nieuprzywilejowanych przestrzeni nazw uzytkownika i montowania"
      exit 77
    fi
    # timeout POZA unshare: wewnatrz potrafi nie zadzialac (shm-exhaustion-repro). unshare i sh
    # robia exec, wiec sygnal limitu trafia w sam serwer; 137 znaczy zawieszenie.
    rc=0
    timeout -s KILL 30 unshare -Umr sh -c 'mount -t tmpfs -o size=512k tmpfs /dev/shm && exec "$0" query.rql -k -m 3' \
      "$XRETRACTOR" </dev/null > out.txt 2> err.txt || rc=$?
    want=$(errno_value ENOBUFS)
    if [ "$rc" -ne "$want" ]; then
      echo "start bez zasobow IPC: kod $rc, oczekiwano $want (ENOBUFS; 137 = zawieszenie)"
      cat out.txt err.txt
      exit 1
    fi
    grep -q 'xretractor: cannot create IPC resources' err.txt || {
      echo "start bez zasobow IPC: brak komunikatu na stderr"
      cat err.txt
      exit 1
    }
    echo "OK: kod $rc i komunikat o zasobach IPC"
    ;;
  *)
    echo "nieznany tryb: $MODE"
    exit 2
    ;;
esac
