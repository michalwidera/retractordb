#!/usr/bin/env bash
# Regresja epoki logicznej zapytań ad hoc. Późny pass-through musi zachować
# bieżący indeks źródła, żeby następny operator indeksowany nie zestawił go
# z historycznym rekordem istniejącego strumienia.
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

xretractor query.rql -m 35 > adhoc-server.out 2> adhoc-server.err &
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

echo OK
