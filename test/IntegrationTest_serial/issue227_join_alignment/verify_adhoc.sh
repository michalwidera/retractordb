#!/usr/bin/env bash
# Regresja epoki logicznej zapytań ad hoc. Późny pass-through musi zachować
# bieżący indeks źródła zarówno w złączeniu, jak i w oknie AGSE.
set -eu

rm -rf temp
mkdir -p temp

server_pid=""
cleanup() {
  xqry -k >/dev/null 2>&1 || true
  if [ -n "$server_pid" ] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

xretractor query.rql -m 45 > adhoc-server.out 2> adhoc-server.err &
server_pid=$!

lock_file="${TMPDIR:-/tmp}/xretractor_service.lock"
for _ in $(seq 1 100); do
  [ -f "$lock_file" ] && break
  sleep 0.02
done
[ -f "$lock_file" ] || {
  echo "ad hoc: serwer nie utworzyl blokady"
  exit 1
}

# Plan ma już rekordy o indeksach większych niż origin=2.
sleep 0.6
xqry -a 'SELECT * STREAM late FROM win'
sleep 0.4
xqry -a 'SELECT * STREAM late_pair FROM late+win'
sleep 0.4
xqry -a 'SELECT * STREAM late_window FROM late@(3,3)'

wait "$server_pid"
server_pid=""

[ -s temp/late_pair ] || {
  echo "ad hoc: late_pair nie zawiera rekordow"
  exit 1
}

# Każdy rekord ma dwa 3-polowe okna. Obie połowy muszą być identyczne:
# late[n] jest pass-through win[n], a nie bieżącą wartością podpisaną origin=2.
od -An -v -w24 -td4 temp/late_pair | awk '
  NF != 6 || $1 == 0 || $1 != $4 || $2 != $5 || $3 != $6 { exit 1 }
  END { if (NR == 0) exit 1 }
' || {
  echo "ad hoc: polaczono rekordy z roznych indeksow logicznych"
  od -An -v -w24 -td4 temp/late_pair
  exit 1
}

[ -s temp/late_window ] || {
  echo "ad hoc: late_window nie zawiera rekordow"
  exit 1
}

# late jest 3-polowym oknem. Kolejne polecenie bierze trzy spłaszczone pola
# kończące się na pierwszym polu bieżącego rekordu: [curr[0], prev[2], prev[1]].
# Wynik musi pozostać pełny zamiast all-NULL powstałego przez odjęcie statycznego
# origin zamiast runtime'owej bazy.
od -An -v -w12 -td4 temp/late_window | awk '
  NF != 3 || $1 == 0 || $2 == 0 || $3 == 0 || $1 != $2 + 3 || $1 != $3 + 2 { exit 1 }
  END { if (NR == 0) exit 1 }
' || {
  echo "ad hoc: AGSE uzyl niewlasciwej bazy indeksu logicznego"
  od -An -v -w12 -td4 temp/late_window
  exit 1
}

echo OK
