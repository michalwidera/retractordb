#!/bin/bash
# Ten sam przeplot co w query.rql, zgloszony przez `xqry -a` do DZIALAJACEGO serwera - razem
# z deklaracjami zrodel. Do 2026-09-27 SELECT dostawal "OK", a serwer ginal w pierwszym slocie
# przeplotu (FatalError "schema mismatch") razem z planem i wszystkimi klientami.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
rm -f temp/*

server_start plan.rql

xqry -a "DECLARE s STRING[8], k INTEGER STREAM ta, 1 FILE 'a.txt'"
xqry -a "DECLARE s STRING[16], k DOUBLE STREAM tb, 1 FILE 'b.txt'"
xqry -a "SELECT * STREAM h FROM ta#tb"

xqry -s h -m 6 >out-adhoc.txt

# `kill -0` zaraz po odpowiedzi nie widzialby smierci odroczonej; wiersze sa dowodem, ze
# przeplot policzyl kilka slotow, a proces ma nadal zyc.
if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer zginal po przeplocie ad-hoc"
  exit 1
fi

# Poczatek zalezy od biezacej osi logicznej, wiec kolejnosci nie przypinamy: kazdy wiersz ma
# byc jednym z szesciu rekordow zrodel - bez obcietego napisu i bez bajtow poprzedniego rekordu.
# xqry konczy kazdy wiersz spacja, stad obciecie przed porownaniem.
rows=$(wc -l <out-adhoc.txt)
if [ "$rows" -ne 6 ]; then
  echo "strumien h oddal $rows z 6 wierszy:"
  cat out-adhoc.txt
  exit 1
fi
while IFS= read -r row; do
  case "$row" in
    "CCCCCCCCCCCCCCCC 1.5" | "ab 1" | "delta 2.5" | "alphabet 2" | "sixteencharslong 3.5" | "x 3") ;;
    *)
      echo "nieoczekiwany wiersz przeplotu ad-hoc: '$row'"
      cat out-adhoc.txt
      exit 1
      ;;
  esac
done < <(sed 's/[[:space:]]*$//' out-adhoc.txt)

xqry -k
server_wait_exit
