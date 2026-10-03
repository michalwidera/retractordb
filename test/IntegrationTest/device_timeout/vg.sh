#!/bin/bash
# #347 pod valgrindem: faza DEVICE z czekaniem w poll(), rekord niepelny przez caly przebieg
# i EOF po danych z --until-eof. Wynik merytoryczny sprawdza run.sh; tu liczy sie czystosc pamieci
# i to, ze przebiegi koncza sie same.
#
# Uzycie: vg.sh [opakowanie...] - opakowanie (valgrind) poprzedza xretractor.
set -e
. "$(dirname "$0")/../portable.sh"

rm -rf vg && mkdir -p vg/temp && cd vg

# Pisarz trzymany na deskryptorze 3: jeden pelny rekord i pol drugiego, wiec kolejne sloty czekaja
# do terminu na rekord, ktory sie nie dokonczy.
mkfifo feed.fifo
exec 3<>feed.fifo
printf 'ABCDEF' >&3
printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM src, 1/5 DEVICE 'feed.fifo' TIMEOUT 0.05" "SELECT src[0] STREAM o FROM src" >wait.rql
run_timeout 120 "$@" xretractor wait.rql -k -r -m 6
exec 3>&-

# Pisarz zapisuje dwa rekordy i odchodzi: z --until-eof przebieg konczy sie sam na EOF.
mkfifo eof.fifo
printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM once, 1/5 DEVICE 'eof.fifo'" "SELECT once[0] STREAM e FROM once" >eof.rql
(printf 'ABCDEFGH' >eof.fifo) &
writer=$!
run_timeout 120 "$@" xretractor eof.rql -k -r -u
wait "$writer"
