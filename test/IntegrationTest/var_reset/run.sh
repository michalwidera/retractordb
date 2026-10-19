#!/bin/bash
# covers: datamodel.data-model.16~1
#
# Utrata stanu zmiennej przy ponownym wczytaniu planu (#531, D6, R2). Plan bez VAR po restarcie
# wraca do poprawnego wyniku po ogonie i oknach; zmienna VAR wraca do wartosci z deklaracji i moze
# pozostac niezgodna ze stanem sprzed restartu az do nastepnego przekroczenia progu.
#
#   1. Zatrzask ustawiony regula: wartosc w pasmie histerezy (220) idzie z ?LATCH = 1.
#   2. `xqry --reset` TEN SAM tekst planu, na danych tylko w pasmie (231) z jednym przekroczeniem
#      (280) dopiero po 50 rekordach. Przed nim zatrzask jest 0 - stan z deklaracji, choc bez
#      resetu wynosilby 1 - a od rekordu po przekroczeniu znow 1.
#
# Rekordy obu epok rozroznia wartosc (220/270 przed, 231/280 po), bo `xqry -s` moze jeszcze
# zlapac koncowke starej epoki.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf temp && mkdir temp
printf '%s\n' 220 270 220 220 > data.txt

server_start plan.rql -k -r

# `xqry -s` konczy wiersz znakami CR LF.
xqry -s st -m 8 | tr -d '\r' > before.txt
grep -Eq '^220 1 ?$' before.txt || {
  echo "zatrzask nie zostal ustawiony regula przed przeladowaniem"
  cat before.txt
  exit 1
}

{
  for _ in $(seq 50); do echo 231; done
  echo 280
  for _ in $(seq 20); do echo 231; done
} > data.txt
xqry --reset plan.rql

# Czekamy na pierwszy rekord nowej epoki; okno 50 rekordow w pasmie to 5 s przy 1/10 s. Proba,
# ktora trafi w wymiane epoki, konczy sie bledem klienta - wtedy probujemy dalej.
i=0
until xqry -s st -m 1 2>/dev/null | grep -Eq '^(231|280) '; do
  i=$((i + 1))
  if [ "$i" -ge 50 ]; then
    echo "nowa epoka planu nie oddala rekordu"
    exit 1
  fi
done
xqry -s st -m 70 | tr -d '\r' > after.txt

awk '
  $1 == 280 { crossed = 1; next }
  $1 != 231 { next }
  !crossed { lost++; if ($2 != 0) bad = bad " przed:" $0 }
  crossed { relatched++; if ($2 != 1) bad = bad " po:" $0 }
  END {
    if (bad != "" || lost == 0 || relatched == 0) {
      printf "zatrzask po przeladowaniu: przed przekroczeniem %d, po nim %d, niezgodne:%s\n", lost, relatched, bad
      exit 1
    }
  }' after.txt || {
  cat after.txt
  exit 1
}

xqry -k
server_wait_exit
