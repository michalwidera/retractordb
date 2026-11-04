#!/bin/bash
# covers: qry.qry.16~1
# `xqry -s -m N` na strumieniu 10 kHz ma sie zakonczyc zaraz po N elementach (#599).
#
# Do 2026-11-04 za petla renderowania stala petla drenazu, ktora zdejmowala z lokalnej kolejki
# producenta po jednym elemencie na milisekunde i konczyla sie dopiero na pustej kolejce. Przy
# serwerze emitujacym szybciej niz wiersz na milisekunde kolejka nie bywala pusta nigdy: klient
# wypisywal N poprawnych wierszy i wisial, az ubil go `timeout`.
#
# Test ma sens tylko przy serwerze, ktory faktycznie emituje szybciej niz ~1 kHz - wolniej petla
# drenazu konczyla sie w losowej chwili i test przechodzilby pusto. Dlatego najpierw mierzy tempo
# emisji i przy zbyt wolnym serwerze konczy sie bledem, a nie sukcesem.
set -e
. "$(dirname "$0")/../serverlib.sh"
rm -rf temp && mkdir -p temp

server_start query.rql -r

out_count() {
  xqry -d -y | awk '$0 == "  - name: out" { own = 1; next } /^  - name: / { own = 0 } own && $1 == "count:" { print $2 }'
}

# Straznik tempa: przyrost liczby rekordow strumienia w ciagu ~1 s. Prog 2000/s to dwukrotnosc
# tempa, przy ktorym petla drenazu (sen 1 ms na element) jeszcze nadazala.
c1=$(out_count)
t1=$(now_ns)
sleep 1
c2=$(out_count)
t2=$(now_ns)
rate=$(((c2 - c1) * 1000000000 / (t2 - t1)))
if [ "$rate" -lt 2000 ]; then
  echo "serwer emituje $rate elementow/s (ponizej 2000/s) - test nie rozstrzyga i konczy sie bledem"
  exit 1
fi
echo "tempo emisji: $rate elementow/s"

for n in 1 5; do
  status=0
  run_timeout 5 xqry -s out -m "$n" >out_$n.txt || status=$?
  if [ "$status" -ne 0 ]; then
    echo "xqry -s out -m $n: kod $status (124 = nie zakonczyl sie w 5 s), wierszy $(wc -l <out_$n.txt)"
    exit 1
  fi
  [ "$(wc -l <out_$n.txt)" -eq "$n" ] || {
    echo "xqry -s out -m $n: $(wc -l <out_$n.txt) wierszy zamiast $n"
    exit 1
  }
done

xqry -k
server_wait_exit
