#!/bin/bash
# #43 pod valgrindem: petla slotow z harmonogramem wzgledem kotwicy - czekanie na DEVICE, praca regul
# i nadrabianie zaleglych slotow po przestoju. Okres 1/4 s, zeby praca pod valgrindem miescila sie
# w slocie z zapasem; progi jak w run.sh, przeskalowane do okresu. Reset, import ad hoc i sygnaly
# sprawdza run.sh - valgrind inaczej szereguje watki i dostarcza im sygnaly.
#
# Uzycie: vg.sh [opakowanie...] - opakowanie (valgrind) poprzedza xretractor.
set -e
. "$(dirname "$0")/../portable.sh"

watcher=""
trap 'if [ -n "$watcher" ]; then kill "$watcher" 2>/dev/null || true; fi' EXIT

rm -rf vg && mkdir vg && cd vg

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

write_ints seq.bin 1 400
write_ints dev.bin 101 140

# Dryf: ~50 ms czekania na pusty DEVICE i ~50 ms reguly w slocie 250 ms.
mkfifo idle.fifo
exec 3<>idle.fifo
mkdir -p dn
printf '%s\n' "STORAGE 'dn'" \
  "DECLARE a INTEGER STREAM dev, 1/4 DEVICE 'idle.fifo' TIMEOUT 0.05" \
  "DECLARE a INTEGER STREAM src, 1/4 BINFILE 'seq.bin'" \
  "SELECT src[0] STREAM o FROM src" \
  "RULE work ON o WHEN o[0] > 0 DO SYSTEM 'sleep 0.05'" >drift.rql
watch_start dn/o
run_timeout 180 "$@" xretractor drift.rql -k -r -m 25
watch_stop
exec 3>&-
python3 ../timing.py drift times.txt dn/o 0.25 0.3

# Przestoj 1,1 s (4,4 okresu) w slocie rekordu 8; DEVICE z zapasem danych w FIFO.
mkfifo feed.fifo
exec 3<>feed.fifo
cat dev.bin >&3
mkdir -p cc
printf '%s\n' "STORAGE 'cc'" \
  "DECLARE a INTEGER STREAM src, 1/4 BINFILE 'seq.bin'" \
  "DECLARE a INTEGER STREAM dev, 1/4 DEVICE 'feed.fifo' TIMEOUT 0.05" \
  "SELECT src[0] STREAM o FROM src" \
  "SELECT dev[0] STREAM d FROM dev" \
  "RULE stall ON o WHEN o[0] = 8 DO SYSTEM 'sleep 1.1'" >catch.rql
watch_start cc/o
run_timeout 180 "$@" xretractor catch.rql -k -r -m 25
watch_stop
exec 3>&-
python3 ../timing.py catchup times.txt cc/o 0.25 1.1 0.15
[ "$(file_size cc/d)" -eq 96 ] || { echo "przestoj: d=$(file_size cc/d) B zamiast 96 B"; exit 1; }
echo "OK"
