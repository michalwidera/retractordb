#!/bin/bash
# Wymiana planu bez magistrali (#521, etap 9b). Kontrakt executorsm: status Unavailable przy
# aktywacji zasobow nowego planu nie konczy procesu - plan wchodzi, a rozlacznosc nazw nie jest
# egzekwowana. Magistrale wylacza obiekt o nazwie segmentu z prawami 0644: kontrola ipcSecurity
# go odrzuca, wiec start i --reset widza Unavailable bez dolaczonego segmentu.
#
# Nazwe segmentu (z wersja ukladu) podaje zywa instancja tej przestrzeni nazw; segment znika
# razem z ostatnia instancja, wiec podlozony obiekt jest usuwany przy kazdym wyjsciu. Bez katalogu
# /dev/shm (Darwin) podmiany nie da sie zrobic: weryfikacja niewykonana, kod 77.
set -e
. "$(dirname "$0")/../serverlib.sh"

if [ ! -d /dev/shm ]; then
  echo "NIEWYKONANA WERYFIKACJA: brak katalogu /dev/shm"
  exit 77
fi
NAME="${RDB_NAMESPACE:?test wymaga przestrzeni nazw}"
rm -rf ./temp && mkdir -p ./temp
server_start plan1.rql --noanykey
SEG=$(ls /dev/shm | grep -E "^xrdbbus_v[0-9]+_${NAME}\$" | head -1 || true)
xqry -k > /dev/null
server_wait_exit
[ -n "$SEG" ] || { echo "zywa instancja przestrzeni $NAME nie ma segmentu magistrali w /dev/shm"; exit 1; }
[ ! -e "/dev/shm/$SEG" ] || { echo "segment $SEG przezyl ostatnia instancje - podmiana niemozliwa"; exit 1; }

trap 'rc=$?; rm -f "/dev/shm/$SEG"; (exit $rc); server_cleanup' EXIT INT TERM
printf 'not a bus segment' > "/dev/shm/$SEG"
chmod 0644 "/dev/shm/$SEG"

rm -rf ./temp && mkdir -p ./temp
server_start plan1.rql --noanykey 2> bus_unavailable.err

# Bez magistrali klient nie widzi instancji w tabeli; serwer odpowiada wylacznie przez IPC.
if bus_own | grep -qE "[ |,]alpha1(,| |\||$)"; then
  echo "magistrala ogloszona mimo odrzuconego segmentu - test niczego nie sprawdza"
  exit 1
fi

xqry --reset plan2.rql > reset.txt 2>&1 || { echo "reset bez magistrali odrzucony:"; cat reset.txt; exit 1; }
i=0
until xqry -s beta1 -m 2 > beta1.txt 2>/dev/null && [ "$(wc -l < beta1.txt)" -ge 2 ]; do
  if [ "$i" -ge 30 ] || ! kill -0 "$_server_pid" 2>/dev/null; then
    echo "plan przyjety bez magistrali nie wszedl (strumien beta1) w ciagu 15 s"
    cat bus_unavailable.err
    exit 1
  fi
  sleep 0.5
  i=$((i + 1))
done
if grep -q 'FATAL' bus_unavailable.err; then
  echo "wymiana planu bez magistrali skonczyla sie bledem krytycznym:"
  cat bus_unavailable.err
  exit 1
fi
xqry -k > /dev/null
server_wait_exit
echo "OK"
