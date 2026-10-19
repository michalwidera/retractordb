#!/bin/bash
# covers: executorsm.executorsm.21~1, qry.qry.15~1
#
# Odczyt biezacych wartosci zmiennych VAR przez `xqry --vars` (#531, etap D).
#
#   1. Serwer z --xqrywait: pierwsza komenda zastaje plan przed pierwszym slotem, wiec widzi
#      wartosci z deklaracji (D6) - po jednej zmiennej kazdego typu D4.
#   2. Po kilku slotach licznik zmieniony regula SET, a przepelnienie INTEGER dalo NULL (D4);
#      to samo w formie YAML (-y).
#   3. --vars z --jsonl to odmowa po stronie klienta.
#   4. Plan bez zmiennych (`--reset`): pusta lista w obu formach.
#   5. Instancja bezczynna (`--reset` pustym planem): wydruk braku planu i kod 0, jak --dir.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf temp && mkdir temp

server_start plan.rql -k -r -x

xqry --vars | tr -d '\r' > declared.txt
cat > expected.txt <<'TABLE'
name  | type     | value
------+----------+-----------
COUNT | INTEGER  | 0
GAIN  | FLOAT    | 0.5
RATIO | RATIONAL | 1/3
BIG   | INTEGER  | 2147483647
TABLE
diff -u expected.txt declared.txt || {
  echo "xqry --vars przed pierwszym slotem nie pokazal wartosci z deklaracji"
  exit 1
}

# Wartosc kolumny `value` zmiennej $1 z wydruku tabeli na stdin.
var_value() { awk -F'|' -v name="$1" '{ gsub(/ /, "") } $1 == name { print $3 }'; }

i=0
until [ "$(xqry --vars | tr -d '\r' | var_value COUNT)" -ge 3 ] 2>/dev/null; do
  i=$((i + 1))
  if [ "$i" -ge 100 ]; then
    echo "licznik COUNT nie doszedl do 3"
    xqry --vars
    exit 1
  fi
  sleep 0.1
done

xqry --vars | tr -d '\r' > running.txt
[ "$(var_value BIG < running.txt)" = null ] || {
  echo "przepelnienie INTEGER w SET nie dalo NULL"
  cat running.txt
  exit 1
}
[ "$(var_value GAIN < running.txt)" = 0.5 ] && [ "$(var_value RATIO < running.txt)" = 1/3 ] || {
  echo "zmienne bez SET zmienily wartosc"
  cat running.txt
  exit 1
}

xqry --vars -y | tr -d '\r' > running.yaml
grep -A2 -x '  - name: BIG' running.yaml | grep -qx '    value: null' &&
  grep -A1 -x '  - name: RATIO' running.yaml | grep -qx '    type: RATIONAL' &&
  head -3 running.yaml | grep -qx 'variables:' || {
  echo "nieoczekiwany dokument YAML"
  cat running.yaml
  exit 1
}

status=0
xqry --vars --jsonl 2> jsonl.err || status=$?
[ "$status" -eq 22 ] && grep -q -- '--jsonl supports' jsonl.err || {
  echo "--vars --jsonl: kod $status, oczekiwana odmowa (22)"
  cat jsonl.err
  exit 1
}

# Po wymianie planu `xqry --vars` moze jeszcze trafic w koncowke starej epoki albo w sama wymiane.
# $1 - oczekiwany wydruk; reszta - argumenty xqry.
await_output() {
  local want="$1"
  shift
  local i=0
  until [ "$(xqry "$@" 2>/dev/null | tr -d '\r')" = "$want" ]; do
    i=$((i + 1))
    if [ "$i" -ge 100 ]; then
      echo "xqry $* nie dal oczekiwanego wydruku:"
      echo "$want"
      echo "otrzymano:"
      xqry "$@"
      exit 1
    fi
    sleep 0.1
  done
}

empty_yaml=$'---\napiVersion: xqry/v1\nvariables: []'

xqry --reset novar.rql
await_output $'name | type | value\n-----+------+------' --vars
await_output "$empty_yaml" --vars -y

: > empty.rql
xqry --reset empty.rql
await_output 'no active plan' --vars
await_output "$empty_yaml" --vars -y

xqry -k
server_wait_exit
