#!/bin/bash
# Przeplot ad-hoc jak w adhoc.sh, ale zrodlo `ta` dociera PRZED krokiem ZERO (#574). Model jest
# opublikowany dla komend wczesniej, niz petla liczy krok ZERO, a ten uzbrajal kazda deklaracje
# planu - takze dolaczona w tym oknie - bez bazy logicznej. Pierwszy nalezny slot ustawia baze
# tylko zrodlu pustemu, wiec `ta` nie dostawalo jej nigdy, a pierwszy rekord przeplotu siegajacy
# do `ta` konczyl proces FatalError-em "logical index base not initialized for 'ta'".
# W CI okno trafial wolny kontener; tutaj otwiera je bramka -x: pierwsza komenda czeka na model,
# a krok ZERO czeka na jej obsluzenie, wiec `ta` wchodzi do planu zawsze przed nim.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
rm -f temp/*

server_start plan.rql -x

xqry -a "DECLARE s STRING[8], k INTEGER STREAM ta, 1 TEXTFILE 'a.txt'"
xqry -a "DECLARE s STRING[16], k DOUBLE STREAM tb, 1 TEXTFILE 'b.txt'"
xqry -a "SELECT * STREAM h FROM ta#tb"

xqry -s h -m 6 >out-adhoc-zero-step.txt

if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer zginal po przeplocie ad-hoc dolaczonym przed krokiem ZERO"
  exit 1
fi

# Te same reguly co w adhoc.sh: szesc wierszy, kazdy jednym z rekordow zrodel, bez kolejnosci.
rows=$(wc -l <out-adhoc-zero-step.txt)
if [ "$rows" -ne 6 ]; then
  echo "strumien h oddal $rows z 6 wierszy:"
  cat out-adhoc-zero-step.txt
  exit 1
fi
while IFS= read -r row; do
  case "$row" in
    "CCCCCCCCCCCCCCCC 1.5" | "ab 1" | "delta 2.5" | "alphabet 2" | "sixteencharslong 3.5" | "x 3") ;;
    *)
      echo "nieoczekiwany wiersz przeplotu ad-hoc: '$row'"
      cat out-adhoc-zero-step.txt
      exit 1
      ;;
  esac
done < <(sed 's/[[:space:]]*$//' out-adhoc-zero-step.txt)

xqry -k
server_wait_exit
