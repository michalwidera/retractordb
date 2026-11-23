#!/bin/bash
# covers: executorsm.executorsm.22~1, executorsm.executorsm.23~1, executorsm.executorsm.24~1, qry.qry.17~1, launcher.xretractor.44~1
#
# Horyzont osi czasu (#640, etap 2).
#
#   0. xretractor -c --horizon: wiersz horyzontu po wyjsciu planu (takze z -r), blad dla planu bez
#      wspolnej podstawy interwalow, odmowa polaczenia z -d.
#   1. timing.horizon = "stop" (domyslnie): plan short.rql dochodzi do horyzontu po 8192 slotach
#      strumienia `a` (--no-clock) i konczy prace bledem silnika z nazwa tego strumienia.
#   2. timing.horizon = "restart": ten sam plan z budzetem -m 40000 przechodzi horyzont dwa razy,
#      za kazdym razem w nowej epoce, i konczy sie kodem 0 po wyczerpaniu budzetu.
#   3. xqry --horizon na planie 1/1000 s w trybie taktowanym: odliczanie, horyzont, chwila, strumien.
#   4. Ad hoc z interwalem 1/2147483647 s ma horyzont 1 s; po ok. 1,2 s pracy jest odrzucany,
#      a silnik liczy dalej i przyjmuje inne zapytanie ad hoc.
#   5. --horizon z --jsonl to odmowa klienta; instancja bezczynna: wydruk braku planu.
set -e
. "$(dirname "$0")/../serverlib.sh"

LOG="${TMPDIR:-/tmp}/xretractor.log"
# Linie dziennika dopisane od chwili zapamietanej w $1 (dziennik dzieli kilka katalogow slotu).
log_since() { tail -n +"$(($1 + 1))" "$LOG" 2>/dev/null; }
log_lines() { wc -l < "$LOG" 2>/dev/null || echo 0; }

rm -rf temp && mkdir temp

# --- 0. xretractor -c --horizon ---
line=$(xretractor -c --horizon -r ms.rql)
[ "$line" = 'time horizon: 2147483.647000 s (24 d 20 h 31 min 23 s) - stream src, interval 1/1000 s' ] || {
  echo "nieoczekiwany wiersz -c --horizon -r: $line"
  exit 1
}
last=$(xretractor -c --horizon short.rql | tail -1)
[ "$last" = 'time horizon: 4.000000 s (4 s) - stream a, interval 1048576/2147483647 s' ] || {
  echo "ostatni wiersz -c --horizon to nie horyzont: $last"
  exit 1
}
status=0
xretractor -c --horizon -r nobase.rql 2> nobase.err || status=$?
[ "$status" -ne 0 ] && grep -q 'no common time base' nobase.err || {
  echo "plan bez wspolnej podstawy: kod $status, oczekiwany blad"
  cat nobase.err
  exit 1
}
status=0
xretractor -c --horizon -d ms.rql > dot.out 2> dot.err || status=$?
[ "$status" -ne 0 ] && grep -q -- '--horizon cannot be combined' dot.err && [ ! -s dot.out ] || {
  echo "-c --horizon -d: kod $status, oczekiwana odmowa bez wyjscia dot"
  cat dot.err
  exit 1
}

# --- 1. stop ---
mark=$(log_lines)
status=0
xretractor short.rql -k -r -f -m 40000 </dev/null 2> stop.err || status=$?
[ "$status" -eq 1 ] || {
  echo "horyzont w trybie stop: kod $status, oczekiwany 1"
  cat stop.err
  exit 1
}
log_since "$mark" | grep 'time horizon of the plan reached' | grep -q "allows at most 8192 slots - stream 'a'" || {
  echo "brak bledu horyzontu z nazwa strumienia 'a'"
  log_since "$mark" | tail -20
  exit 1
}

# --- 2. restart ---
mark=$(log_lines)
status=0
xretractor short.rql -k -r -f -m 40000 -g restart.toml </dev/null 2> restart.err || status=$?
[ "$status" -eq 0 ] || {
  echo "horyzont w trybie restart: kod $status, oczekiwany 0"
  cat restart.err
  log_since "$mark" | tail -20
  exit 1
}
restarts=$(log_since "$mark" | grep -c 'restarting the plan in a new epoch' || true)
[ "$restarts" -eq 2 ] || {
  echo "oczekiwane 2 restarty na horyzoncie, jest $restarts"
  log_since "$mark" | tail -20
  exit 1
}

# --- 3. xqry --horizon ---
server_start ms.rql -k -r

# Wartosc pola $1 raportu --horizon (druga kolumna).
field() { xqry --horizon | tr -d '\r' | awk -v key="$1" '$1 == key { print $2 }'; }

# Czekanie, az os przejdzie 1,2 s - za horyzontem zapytania ad hoc z punktu 4.
i=0
until awk -v t="$(field time)" 'BEGIN { exit !(t > 1.2) }'; do
  i=$((i + 1))
  if [ "$i" -ge 100 ]; then
    echo "os nie doszla do 1,2 s"
    xqry --horizon
    exit 1
  fi
  sleep 0.1
done

xqry --horizon | tr -d '\r' > report.txt
grep -Eqx 'remaining 24 d 20 h 31 min [0-9]+ s' report.txt &&
  grep -qx 'horizon   2147483.647000 s' report.txt &&
  grep -Eqx 'time      [0-9]+\.[0-9]{6} s' report.txt &&
  grep -qx 'stream    src (interval 1/1000 s)' report.txt || {
  echo "nieoczekiwany raport --horizon"
  cat report.txt
  exit 1
}

# --- 4. odmowa ad hoc poza horyzontem ---
status=0
xqry -a "DECLARE v INTEGER STREAM fast, 1/2147483647 TEXTFILE 'data.txt'" > adhoc.out 2>&1 || status=$?
[ "$status" -ne 0 ] &&
  grep -q "Rejected: the time horizon of the plan with this query is 1.000000 s (stream 'fast', interval 1/2147483647 s)" adhoc.out || {
  echo "ad hoc za horyzontem: kod $status, oczekiwana odmowa"
  cat adhoc.out
  exit 1
}
before=$(field time)
xqry -a "DECLARE v INTEGER STREAM slow, 1/10 TEXTFILE 'data.txt'"
sleep 0.3
after=$(field time)
awk -v a="$before" -v b="$after" 'BEGIN { exit !(b > a) }' || {
  echo "silnik nie liczy po odmowie: chwila $before -> $after"
  exit 1
}
xqry -d | grep -q '^slow ' || {
  echo "zapytanie ad hoc po odmowie nie weszlo do planu"
  xqry -d
  exit 1
}

# --- 5. --jsonl i instancja bezczynna ---
status=0
xqry --horizon --jsonl 2> jsonl.err || status=$?
[ "$status" -eq 22 ] && grep -q -- '--jsonl supports' jsonl.err || {
  echo "--horizon --jsonl: kod $status, oczekiwana odmowa (22)"
  cat jsonl.err
  exit 1
}

: > empty.rql
xqry --reset empty.rql
i=0
until [ "$(xqry --horizon 2>/dev/null | tr -d '\r')" = 'no active plan' ]; do
  i=$((i + 1))
  if [ "$i" -ge 100 ]; then
    echo "--horizon bez planu nie dal wydruku braku planu"
    xqry --horizon
    exit 1
  fi
  sleep 0.1
done

xqry -k
server_wait_exit
