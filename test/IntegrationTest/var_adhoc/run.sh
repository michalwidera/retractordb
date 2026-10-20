#!/bin/bash
# covers: rqlparser.plan-variable-names.1~1, rqlparser.plan-variable-names.2~1, datamodel.data-model.16~1
#
# Zmienne VAR w kanale ad hoc i cykl zycia planu (#531, etap C: D7, D6, D1, D3, D4).
#
#   1. Ad hoc SELECT czyta zmienna planu: ?LATCH w strumieniu ad hoc jest w kazdym rekordzie rowne
#      polu ?LATCH strumienia planu z tego samego slotu (D1 - stala w slocie).
#   2. Ad hoc RULE ... DO SET pisze zmienna planu: licznik rosnie o 1 na rekord (zapis widoczny od
#      nastepnego slotu, D1).
#   3. Konflikt zapisu (D3): regula ad hoc ma numer w tekscie za kazda regula planu i wygrywa.
#   4. Odmowy ad hoc (D4, stala LET, kolizja nazwy, deklaracja VAR) nie zmieniaja planu ani wartosci.
#   5. Odmowa `xqry --reset` nie zmienia planu, regul ad hoc ani wartosci.
#   6. `xqry --reset` na plan z tymi samymi nazwami pod innymi indeksami: wartosci z nowej deklaracji
#      (D6), reguly ad hoc poprzedniego planu znikaja, ad hoc widzi nazwy nowego planu.
#   7. `xqry --reset` na plan bez VAR: nazwy zmiennych poprzednich planow nie przeciekaja.
#
# `xqry -s` konczy wiersz znakami CR LF, stad `tr -d '\r'`.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf temp temp_bad temp_b temp_c && mkdir temp temp_bad temp_b temp_c
# Cykl 6 rekordow: 270 ustawia zatrzask, 180 go zdejmuje, 220 trzyma stan.
printf '%s\n' 220 270 220 220 180 220 > data.txt

expect_refusal() { # <fragment komunikatu> <zapytanie ad hoc>
  local needle="$1" query="$2" out rc
  set +e
  out=$(xqry -a "$query" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    echo "przyjeto zapytanie, ktore mialo zostac odrzucone: $query"
    exit 1
  fi
  case "$out" in
    *"$needle"*) ;;
    *)
      echo "inny powod odmowy niz oczekiwany dla: $query"
      echo "oczekiwano fragmentu: $needle"
      echo "odpowiedz: $out"
      exit 1
      ;;
  esac
}

# Czeka na pierwszy rekord strumienia nowej epoki planu. Proba, ktora trafi w wymiane epoki, konczy
# sie bledem klienta - wtedy probujemy dalej.
wait_stream() {
  local i=0
  until xqry -s "$1" -m 1 > /dev/null 2>&1; do
    i=$((i + 1))
    if [ "$i" -ge 50 ]; then
      echo "strumien $1 nie oddal rekordu"
      exit 1
    fi
    sleep 0.1
  done
}

# Ostatnia wartosc licznika ?COUNT (pole 3 strumienia st).
last_count() {
  xqry -s st -m 3 | tr -d '\r' | awk 'END { print $3 }'
}

server_start plan.rql -k -r

# --- 1. Odczyt zmiennej planu w ad hoc SELECT. ---
xqry -a "SELECT st[0], st[1], ?LATCH STREAM probe FROM st"
xqry -s probe -m 20 | tr -d '\r' > probe.txt
awk '
  $2 != $3 { bad = bad " " $0 }
  $3 == 1 { on++ }
  $3 == 0 { off++ }
  END {
    if (bad != "" || on == 0 || off == 0) {
      printf "ad hoc ?LATCH rozne od pola planu albo bez obu stanow (1: %d, 0: %d):%s\n", on, off, bad
      exit 1
    }
  }' probe.txt || {
  cat probe.txt
  exit 1
}

# --- 2. Regula SET dolaczona ad hoc. ---
xqry -a "RULE count ON st WHEN 1 = 1 DO SET ?COUNT = ?COUNT + 1"
xqry -s st -m 20 | tr -d '\r' > count.txt
awk '
  NR > 1 && $3 != prev + 1 { bad = bad " " prev "->" $3 }
  { prev = $3 }
  END {
    if (bad != "" || prev <= 0) {
      printf "licznik ad hoc nie rosnie o 1 na rekord:%s (ostatni %s)\n", bad, prev
      exit 1
    }
  }' count.txt || {
  cat count.txt
  exit 1
}

# --- 3. Konflikt zapisu: regula ad hoc za latch_on w tekscie wygrywa (D3). ---
# Prog literalem: stala LET planu nie jest widoczna w ad hoc (#440 O1, faza 4).
xqry -a "RULE force ON st WHEN st[0] >= 250 DO SET ?LATCH = 7"
xqry -s st -m 20 | tr -d '\r' > force.txt
awk '
  crossed && $2 == 1 { bad = bad " " $0 }
  crossed && $2 == 7 { forced++ }
  $1 == 270 { crossed = 1 }
  END {
    if (bad != "" || forced == 0) {
      printf "po przekroczeniu progu zatrzask nie jest 7 (7: %d):%s\n", forced, bad
      exit 1
    }
  }' force.txt || {
  cat force.txt
  exit 1
}

# --- 4. Odmowy ad hoc nie zmieniaja planu ani wartosci zmiennych. ---
before=$(last_count)
latch_line=$(grep -n '^VAR LATCH' plan.rql | cut -d: -f1)
expect_refusal "expression is FLOAT, but variable ?LATCH is INTEGER" "RULE bad ON st WHEN 1 = 1 DO SET ?LATCH = st[0] * 0.5"
expect_refusal "variable ?T_ON in DO SET is not declared" "RULE bad ON st WHEN 1 = 1 DO SET ?T_ON = 1"
expect_refusal "constant ?T_ON in expression is not declared" "SELECT st[0] * ?T_ON STREAM bad FROM st"
expect_refusal "variable ?LATCH is already declared on line $latch_line" "VAR LATCH = 1"
expect_refusal "'VAR' is not supported" "VAR NEW = 1"
if xqry -d | grep -q 'bad'; then
  echo "odrzucone zapytanie ad hoc zmienilo plan serwera"
  xqry -d
  exit 1
fi
after=$(last_count)
if [ "$after" -le "$before" ]; then
  echo "licznik po odmowach ad hoc nie rosnie dalej: przed $before, po $after"
  exit 1
fi

# --- 5. Odmowa `--reset` nie zmienia planu, regul ad hoc ani wartosci. ---
status=0
xqry --reset bad.rql > bad_out.txt 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q 'expression is FLOAT, but variable ?LATCH is INTEGER' bad_out.txt; then
  echo "wadliwy plan nie zostal odrzucony z powodem D4 (kod $status)"
  cat bad_out.txt
  exit 1
fi
again=$(last_count)
if [ "$again" -le "$after" ]; then
  echo "licznik po odrzuconym --reset nie rosnie dalej: przed $after, po $again"
  exit 1
fi

# --- 6. `--reset` na plan B: wartosci z jego deklaracji, nazwy pod nowymi indeksami. ---
xqry --reset plan_b.rql
wait_stream st_b
xqry -s st_b -m 10 | tr -d '\r' > plan_b.txt
awk '
  $2 != 3 || $3 != 100 || $4 != 5 { bad = bad " " $0 }
  END {
    if (bad != "" || NR == 0) {
      printf "plan B nie liczy z wartosci wlasnych deklaracji:%s\n", bad
      exit 1
    }
  }' plan_b.txt || {
  cat plan_b.txt
  exit 1
}
xqry -a "SELECT st_b[0], ?COUNT STREAM probe_b FROM st_b"
xqry -s probe_b -m 3 | tr -d '\r' > probe_b.txt
awk '$2 != 100 { bad = 1 } END { exit bad || NR == 0 }' probe_b.txt || {
  echo "ad hoc w planie B nie czyta ?COUNT z jego deklaracji"
  cat probe_b.txt
  exit 1
}

# --- 7. `--reset` na plan C bez VAR: nazwy nie przeciekaja. ---
xqry --reset plan_c.rql
wait_stream st_c
expect_refusal "constant ?COUNT in expression is not declared" "SELECT st_c[0], ?COUNT STREAM leak FROM st_c"
expect_refusal "variable ?LATCH in DO SET is not declared" "RULE leak ON st_c WHEN 1 = 1 DO SET ?LATCH = 1"

xqry -k
server_wait_exit
