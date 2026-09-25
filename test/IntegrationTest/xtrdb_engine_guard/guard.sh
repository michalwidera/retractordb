#!/bin/bash
# Straznik "silnik dziala" w xtrdb (issue #253).
#
# xtrdb odmawia pracy, gdy w obowiazujacym katalogu blokad ktos TRZYMA blokade z rodziny
# xretractor_service*.lock. Dawniej sprawdzal jedna pisownie (xretractor_service.lock w TMPDIR),
# wiec mijal kazda instancje nazwana i kazda blokade przeniesiona przez paths.lock_dir - a nazwana
# instancja to tryb zalecany.
#
#   a) kontrola dodatnia: silnika brak, obok lezy porzucony plik z rodziny - xtrdb pisze. Bez niej
#      odmowa w (b) i (c) niczego nie dowodzi: straznik odmawiajacy zawsze tez by je zdal.
#   b) zyje `xretractor --name alfa` - xtrdb odmawia, a magazyn sie nie zmienia; po zatrzymaniu
#      alfy xtrdb znow pisze.
#   c) zyje `--name beta` z blokada w paths.lock_dir, poza TMPDIR - xtrdb czytajacy te sama
#      konfiguracje odmawia.
set -e
. "$(dirname "$0")/../portable.sh"

# Wlasny TMPDIR: blokada instancji uruchomionej na tej maszynie poza ctest nie moze trafic do
# domyslnego katalogu blokad tego testu - kontrola dodatnia wypadlaby wtedy falszywie.
export TMPDIR="$PWD/tmp"
# Warstwa uzytkownika konfiguracji wylacznie z katalogu testu: paths.lock_dir z ~/.config
# przenioslby blokady gdzie indziej. Katalogu cfg_none celowo nie ma.
export XDG_CONFIG_HOME="$PWD/cfg_none"
LOCKDIR_CFG="$PWD/cfg_lockdir"
LOCKDIR="$PWD/locks"

LOCK_A="$TMPDIR/xretractor_service.alfa.lock"
LOCK_B="$LOCKDIR/xretractor_service.beta.lock"
STALE="$TMPDIR/xretractor_service.stale.lock"

rm -rf "$TMPDIR" "$LOCKDIR_CFG" "$LOCKDIR" guarded guarded.* guarddst guarddst.*
mkdir -p "$TMPDIR" "$LOCKDIR" "$LOCKDIR_CFG/retractor"
cat >"$LOCKDIR_CFG/retractor/retractor.toml" <<EOF
[paths]
lock_dir = "$LOCKDIR"
EOF

pid_a=""
pid_b=""

stop_server() {
  local name="$1" pid="$2" waited=0
  xqry --server "$name" -k >/dev/null 2>&1 || true
  while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt 50 ]; do
    sleep 0.1
    waited=$((waited + 1))
  done
  kill -KILL "$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true
}

cleanup() {
  local status=$?
  trap - EXIT INT TERM
  if [ -n "$pid_a" ]; then stop_server alfa "$pid_a"; fi
  if [ -n "$pid_b" ]; then stop_server beta "$pid_b"; fi
  rm -f "$LOCK_A" "$LOCK_B" "$STALE"
  # Bramka higieny jak w multiserver_named: obiekty IPC tych instancji nie maja prawa zostac.
  # shm_list konczy sie kodem 2, gdy katalogu obiektow IPC nie da sie ustalic - kontrola jest
  # wtedy jawnie POMINIETA, a nie uznana za zdana.
  local leftovers shm_status=0
  leftovers=$(shm_list 'alfa|beta') || shm_status=$?
  if [ "$shm_status" -ne 0 ]; then
    echo "POMINIETO: higiena IPC niesprawdzalna na tej platformie"
  elif [ -n "$leftovers" ]; then
    echo "higiena: zostaly obiekty IPC:"
    echo "$leftovers"
    status=1
  fi
  exit "$status"
}
trap cleanup EXIT INT TERM

wait_for_lock() {
  local lock="$1" pid="$2" i=0
  while [ "$i" -lt 200 ]; do
    grep -qx "PID: $pid" "$lock" 2>/dev/null && return 0
    kill -0 "$pid" 2>/dev/null || break
    sleep 0.05
    i=$((i + 1))
  done
  echo "serwer $pid nie przejal blokady $lock w ciagu 10 s"
  return 1
}

# (a) kontrola dodatnia
: >"$STALE"
xtrdb noprompt <append.script >a.out 2>&1 || {
  echo "(a) xtrdb odmowil bez dzialajacego silnika"
  cat a.out
  exit 1
}
size_a=$(file_size guarded)
if [ "$size_a" -eq 0 ]; then
  echo "(a) xtrdb nie dopisal rekordu"
  exit 1
fi
rm -f "$STALE"

# (b) instancja nazwana w domyslnym katalogu blokad
xretractor query.rql --noanykey --name alfa </dev/null >alfa.log 2>&1 &
pid_a=$!
wait_for_lock "$LOCK_A" "$pid_a" || {
  cat alfa.log
  exit 1
}
set +e
xtrdb noprompt <append.script >b.out 2>&1
rc_b=$?
set -e
if [ "$rc_b" -eq 0 ] || ! grep -q 'xretractor is running' b.out; then
  echo "(b) xtrdb nie odmowil przy zywej instancji alfa (rc=$rc_b)"
  cat b.out
  exit 1
fi
if [ "$(file_size guarded)" -ne "$size_a" ]; then
  echo "(b) xtrdb zmienil magazyn mimo odmowy"
  exit 1
fi
stop_server alfa "$pid_a"
pid_a=""
xtrdb noprompt <append.script >b_after.out 2>&1 || {
  echo "(b) xtrdb odmawia po zatrzymaniu alfy"
  cat b_after.out
  exit 1
}
size_b=$(file_size guarded)
if [ "$size_b" -le "$size_a" ]; then
  echo "(b) xtrdb nie dopisal rekordu po zatrzymaniu alfy"
  exit 1
fi

# (c) blokada przeniesiona przez paths.lock_dir
XDG_CONFIG_HOME="$LOCKDIR_CFG" xretractor query.rql --noanykey --name beta </dev/null >beta.log 2>&1 &
pid_b=$!
wait_for_lock "$LOCK_B" "$pid_b" || {
  cat beta.log
  exit 1
}
# Blokada ma lezec WYLACZNIE w paths.lock_dir - inaczej odmowa nie dowodzi, ze xtrdb czyta
# konfiguracje, bo znalazlby ja takze w TMPDIR.
if [ -e "$TMPDIR/xretractor_service.beta.lock" ]; then
  echo "(c) blokada beta lezy w TMPDIR, a nie tylko w paths.lock_dir"
  exit 1
fi
set +e
XDG_CONFIG_HOME="$LOCKDIR_CFG" xtrdb noprompt <append.script >c.out 2>&1
rc_c=$?
set -e
if [ "$rc_c" -eq 0 ] || ! grep -q 'xretractor is running' c.out; then
  echo "(c) xtrdb nie odmowil przy instancji beta z blokada w paths.lock_dir (rc=$rc_c)"
  cat c.out
  exit 1
fi
if [ "$(file_size guarded)" -ne "$size_b" ]; then
  echo "(c) xtrdb zmienil magazyn mimo odmowy"
  exit 1
fi
