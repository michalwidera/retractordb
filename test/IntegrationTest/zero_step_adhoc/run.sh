#!/bin/bash
# Krok ZERO zbiera string_view do nazw deklaracji. Import ad-hoc przebudowuje zywy plan,
# wiec musi czekac az processZeroStep i broadcast skoncza korzystac z tych widokow.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf temp
mkdir -p temp
gate="$PWD/temp/zero_step_gate"
export RDB_FAULT_ZERO_STEP_GATE="$gate"
server_start query.rql -k
unset RDB_FAULT_ZERO_STEP_GATE

wait_for_file() {
  local path="$1" i=0
  while [ ! -e "$path" ] && [ "$i" -lt 200 ]; do
    kill -0 "$_server_pid" 2>/dev/null || break
    sleep 0.05
    i=$((i + 1))
  done
  if [ ! -e "$path" ]; then
    echo "brak znacznika $path"
    return 1
  fi
}

# Hak zatrzymuje ZERO po zebraniu widokow. Drugi znacznik dowodzi, ze handler
# przyjal ad-hoc przed zwolnieniem blokady, a nie ze klient tylko wystartowal.
wait_for_file "$gate"
(
  xqry -a 'SELECT src[0]+1 STREAM extra FROM src' > adhoc.out 2>&1
  echo done > temp/adhoc_done
) &
client_pid=$!
wait_for_file "$gate.adhoc"

# Na silniku bez blokady epoki import konczy sie, gdy ZERO nadal czeka na .release.
# Na poprawnym silniku handler czeka na plan_epoch_mutex przez caly ten okres.
sleep 1
if [ -e temp/adhoc_done ]; then
  echo "import ad-hoc zakonczyl sie przed koncem kroku ZERO"
  cat adhoc.out
  exit 1
fi
touch "$gate.release"
wait "$client_pid"

xqry -d > directory.out
grep -qw extra directory.out || {
  echo "import ad-hoc nie pojawil sie w planie"
  cat adhoc.out directory.out
  exit 1
}
test -e temp/adhoc_done
kill -0 "$_server_pid"
xqry -k > /dev/null
server_wait_exit
