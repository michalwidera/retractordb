#!/bin/bash
# #43: sloty sa planowane wzgledem stalej kotwicy czasu monotonicznego w kazdym trybie taktowanym.
# Termin slotu = kotwica epoki + czas logiczny slotu, wiec czas pracy slotu (obliczenia, czekanie na
# DEVICE) nie przesuwa nastepnych slotow. Harmonogram mierzy z zewnatrz timing.py, po chwilach
# pojawiania sie rekordow w magazynie. Dawny sen wzgledny (sleep_for(okres)) oblewa punkty 1-3 i 6.
#
# Test sprawdza po kolei:
#  1. dryf: praca w slocie krotsza od okresu (czekanie na pusty DEVICE i regula DO SYSTEM 'sleep')
#     nie powoduje narastania opoznienia,
#  2. --realtime: ten sam harmonogram i ten sam wynik co bez opcji, takze bez uprawnien RT,
#  3. jeden dluzszy przestoj: zalegle sloty ida kolejno bez snu, potem powrot na pierwotna siatke;
#     artefakty BINFILE i DEVICE sa bajtowo rowne przebiegowi --no-clock, czyli nic nie pominieto
#     ani nie przestawiono,
#  4. reset planu: nowa epoka ma wlasna kotwice i nie rusza seria zaleglych slotow,
#  5. import ad hoc nowej szybkosci: dotychczasowy strumien zostaje na swojej siatce, a nowy lezy
#     na osi tej samej kotwicy,
#  6. zatrzymanie w trakcie snu konczy przebieg od razu, bez slotu, ktorego termin nie nadszedl.
#     Na Linuksie sygnal procesu trafia do watku glownego; gdzie indziej moze trafic do watku
#     komunikacyjnego, ktory snu nie przerwie - tam sprawdzana jest tylko gorna granica.
# Progi sa luzne wobec jitteru i obciazenia CI, a kilkukrotnie ponizej efektu snu wzglednego.
set -e
. "$(dirname "$0")/../serverlib.sh"

# Odpytywanie w tle konczy plik `stop`; przy porazce w polowie zabija je sprzatanie.
watcher=""
slot_cleanup() {
  local status=$?
  if [ -n "$watcher" ]; then kill "$watcher" 2>/dev/null || true; fi
  (exit "$status")
  server_cleanup
}
trap slot_cleanup EXIT INT TERM

rm -rf work && mkdir work && cd work

# Rekordy INTEGER o wartosciach od $2 do $3 w kolejnosci bajtow maszyny.
write_ints() {
  python3 -c "import sys, struct; open(sys.argv[1], 'wb').write(b''.join(struct.pack('=i', v) for v in range(int(sys.argv[2]), int(sys.argv[3]) + 1)))" "$@"
}
watch_start() {
  rm -f stop times.txt
  python3 ../timing.py watch stop 4 times.txt "$@" </dev/null >/dev/null 2>&1 &
  watcher=$!
}
watch_stop() {
  touch stop
  wait "$watcher"
  watcher=""
}
check() { python3 ../timing.py "$@"; }
# wait_records <plik> <liczba> - czeka do 30 s, az w magazynie bedzie tyle rekordow INTEGER.
wait_records() {
  local i=0
  until [ -f "$1" ] && [ "$(file_size "$1")" -ge $(($2 * 4)) ]; do
    if [ "$i" -ge 300 ]; then
      echo "$1: brak $2 rekordow w ciagu 30 s"
      exit 1
    fi
    sleep 0.1
    i=$((i + 1))
  done
}

write_ints seq.bin 1 400
write_ints dev.bin 101 140

# (1) Dryf. Okres 0,1 s; w kazdym slocie ~25 ms czekania na pusty DEVICE (pisarz trzymany na
# deskryptorze 3, danych brak) i ~25 ms reguly. Sen wzgledny przesuwa kazdy slot o te ~55 ms,
# czyli o ~0,55 s miedzy tercjami 30 slotow; limit 0,2 s.
mkfifo idle.fifo
exec 3<>idle.fifo
drift_plan() {
  printf '%s\n' "STORAGE '$1'" \
    "DECLARE a INTEGER STREAM dev, 1/10 DEVICE 'idle.fifo' TIMEOUT 0.025" \
    "DECLARE a INTEGER STREAM src, 1/10 BINFILE 'seq.bin'" \
    "SELECT src[0] STREAM o FROM src" \
    "RULE work ON o WHEN o[0] > 0 DO SYSTEM 'sleep 0.025'"
}
mkdir -p dn dt
drift_plan dn >drift-n.rql
watch_start dn/o
run_timeout 60 xretractor drift-n.rql -k -r -m 31 >/dev/null
watch_stop
check drift times.txt dn/o 0.1 0.2

# (2) To samo z --realtime. Bez uprawnien RT opcja konczy sie na wypisie zgodnosci, a harmonogram
# ma byc ten sam.
drift_plan dt >drift-t.rql
watch_start dt/o
run_timeout 60 xretractor drift-t.rql -k -r -t -m 31 >/dev/null
watch_stop
check drift times.txt dt/o 0.1 0.2
cmp dn/o dt/o
exec 3>&-

# (3) Przestoj. Regula zatrzymuje slot rekordu 8 na 0,45 s, czyli na 4,5 okresu. Zrodlo DEVICE ma
# caly zapas danych w FIFO, wiec zalegle sloty nie czekaja na nie. Sen wzgledny zostawia trwale
# przesuniecie o caly przestoj i nie ma serii bez snu.
catch_plan() {
  printf '%s\n' "STORAGE '$1'" \
    "DECLARE a INTEGER STREAM src, 1/10 BINFILE 'seq.bin'" \
    "DECLARE a INTEGER STREAM dev, 1/10 DEVICE 'feed.fifo' TIMEOUT 0.05" \
    "SELECT src[0] STREAM o FROM src" \
    "SELECT dev[0] STREAM d FROM dev" \
    "RULE stall ON o WHEN o[0] = 8 DO SYSTEM 'sleep 0.45'"
}
mkfifo feed.fifo
mkdir -p cc cf
catch_plan cc >catch.rql
catch_plan cf >catch-f.rql
exec 3<>feed.fifo
cat dev.bin >&3
watch_start cc/o
run_timeout 60 xretractor catch.rql -k -r -m 25 >/dev/null
watch_stop
exec 3>&-
check catchup times.txt cc/o 0.1 0.45 0.1
exec 3<>feed.fifo
cat dev.bin >&3
run_timeout 60 xretractor catch-f.rql -k -r -f -m 25 >/dev/null
exec 3>&-
cmp cc/o cf/o
cmp cc/d cf/d
if [ "$(file_size cc/o)" -ne 96 ] || [ "$(file_size cc/d)" -ne 96 ]; then
  echo "przestoj: o=$(file_size cc/o) B, d=$(file_size cc/d) B zamiast po 96 B (24 sloty)"
  exit 1
fi

# (4) Reset planu po ~1,5 s pracy. Nowa epoka liczy sloty od wlasnej kotwicy; gdyby wziela kotwice
# poprzedniej, pierwsze ~15 slotow poszloby seria bez snu.
mkdir -p ra rb
printf '%s\n' "STORAGE 'ra'" "DECLARE a INTEGER STREAM src, 1/10 BINFILE 'seq.bin'" "SELECT src[0] STREAM o FROM src" >reset-a.rql
printf '%s\n' "STORAGE 'rb'" "DECLARE a INTEGER STREAM src2, 1/10 BINFILE 'seq.bin'" "SELECT src2[0] STREAM r FROM src2" >reset-b.rql
server_start reset-a.rql -k
sleep 1.5
watch_start rb/r
xqry --reset reset-b.rql
wait_records rb/r 15
xqry -k >/dev/null
server_wait_exit
watch_stop
check grid times.txt rb/r 0.1 0.3

# (5) Import ad hoc szybkosci 1/10 do planu 1/4 po ~1,3 s. Os czasu nie jest przewijana: strumien o
# zostaje na swojej siatce, a strumien g z importu lezy na siatce 0,1 s od tej samej kotwicy.
mkdir -p ah
printf '%s\n' "STORAGE 'ah'" "DECLARE a INTEGER STREAM src, 1/4 BINFILE 'seq.bin'" "SELECT src[0] STREAM o FROM src" >adhoc.rql
watch_start ah/o ah/g
server_start adhoc.rql -k
sleep 1.3
xqry -a "DECLARE a INTEGER STREAM fast, 1/10 BINFILE 'seq.bin'"
xqry -a "SELECT fast[0]+1 STREAM g FROM fast"
wait_records ah/o 14
xqry -k >/dev/null
server_wait_exit
watch_stop
check grid times.txt ah/o 0.25 0.1
check grid times.txt ah/g 0.1 0.1
check phase times.txt ah/g 0.1 ah/o 0.25 0.04

# (6) SIGTERM w trakcie snu przed pierwszym slotem okresu 4 s. Sen wzgledny dosypial do terminu
# i liczyl ten slot; sen absolutny zglasza przerwanie, a petla konczy epoke przed slotem.
mkdir -p st
printf '%s\n' "STORAGE 'st'" "DECLARE a INTEGER STREAM slow, 4 BINFILE 'seq.bin'" "SELECT slow[0] STREAM o FROM slow" >stop.rql
server_start stop.rql -k
sleep 1
t0=$(now_ns)
kill -TERM "$_server_pid"
server_wait_exit
t1=$(now_ns)
stop_ms=$(((t1 - t0) / 1000000))
echo "zatrzymanie w trakcie snu: ${stop_ms} ms"
if [ "$(uname -s)" = Linux ]; then
  [ "$stop_ms" -lt 2000 ] || { echo "SIGTERM czekal do terminu slotu: ${stop_ms} ms"; exit 1; }
  [ "$(file_size st/o)" -eq 0 ] || { echo "zatrzymanie policzylo slot przed terminem: $(file_size st/o) B"; exit 1; }
else
  [ "$stop_ms" -lt 6000 ] || { echo "SIGTERM nie zatrzymal przebiegu: ${stop_ms} ms"; exit 1; }
fi
echo "OK"
