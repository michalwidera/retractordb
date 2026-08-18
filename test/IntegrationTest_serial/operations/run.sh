#!/bin/bash
# Przeplot dwoch zrodel o roznym takcie: core0 rosnie monotonicznie, core1 tez,
# a miedzy kolejnymi wartosciami core1 musza pasc dokladnie dwie wartosci core0.
#
# Logika wyniesiona z `bash -c` w add_test: srednik jest w CMake separatorem
# listy, a oprawa serwerowa wymaga synchronizacji, ktorej w jednej linii nie da
# sie uczciwie zapisac (zob. ../serverlib.sh).
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -f ./*.meta ./*.desc str1 str2 str3 core0 core1
server_start query.rql -k -x
xqry -s str3 -k -m 9 > out.txt
server_wait_exit

awk 'BEGIN{lc1=0;lc0=0;between=0;seen_c1=0}
     /^[0-9]/{v=$1+0;
       if(v>=100){
         if(v<=lc1){print "core1 not increasing at "v; exit 1};
         if(seen_c1 && between!=2){print "wrong core0 count between core1: "between; exit 1};
         lc1=v; between=0; seen_c1=1
       } else if(v>0){
         if(v<=lc0){print "core0 not increasing at "v; exit 1};
         lc0=v; between++
       }}
     END{if(lc1<110){print "too few core1 values: "lc1; exit 1}}' out.txt
