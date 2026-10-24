#!/bin/bash
# Straznik "silnik dziala" w xtrdb bez pliku blokady (#573).
#
# Plik blokady silnika lezy w katalogu tymczasowym. Czyszczenie wedlug wieku (systemd-tmpfiles,
# periodic na macOS) kasuje go spod zywego serwera, a PrivateTmp chowa go przed xtrdb. Drugim
# dowodem zycia jest slot na magistrali wlasnej przestrzeni RDB_NAMESPACE.
#
#   a) kontrola dodatnia: silnika brak - xtrdb pisze. Bez niej odmowa w (b) i (c) niczego nie
#      dowodzi: straznik odmawiajacy zawsze tez by je zdal.
#   b) zyje xretractor, jego plik blokady skasowany - xtrdb odmawia, magazyn sie nie zmienia.
#   c) w miejscu skasowanego pliku lezy nowy, nietrzymany (podmiana) - xtrdb nadal odmawia.
#   d) po zatrzymaniu silnika xtrdb znow pisze: odmowe dawal zywy slot, nie cos innego.
#
# Katalog blokad to prywatny TMPDIR przestrzeni z puli (makro add_test), wiec kasowany plik nalezy
# wylacznie do serwera tego testu.
set -e
. "$(dirname "$0")/../portable.sh"

if [ -z "$RDB_NAMESPACE" ] || [ -z "$TMPDIR" ]; then
  echo "test wymaga RDB_NAMESPACE i TMPDIR z puli przestrzeni nazw (uruchom przez ctest)"
  exit 1
fi

LOCK="$TMPDIR/xretractor_service.$RDB_NAMESPACE.lock"
pid=""

stop_server() {
  local waited=0
  xqry --server "$RDB_NAMESPACE" -k >/dev/null 2>&1 || true
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
  if [ -n "$pid" ]; then stop_server; fi
  rm -f "$LOCK"
  exit "$status"
}
trap cleanup EXIT INT TERM

refused() {
  local label="$1" out="$2" size="$3" rc
  set +e
  xtrdb noprompt <append.script >"$out" 2>&1
  rc=$?
  set -e
  if [ "$rc" -eq 0 ] || ! grep -q 'xretractor is running' "$out"; then
    echo "($label) xtrdb nie odmowil przy zywym silniku (rc=$rc)"
    cat "$out"
    exit 1
  fi
  if [ "$(file_size guarded)" -ne "$size" ]; then
    echo "($label) xtrdb zmienil magazyn mimo odmowy"
    exit 1
  fi
}

rm -f guarded guarded.* "$LOCK"

# (a) kontrola dodatnia
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

# (b) plik blokady skasowany pod zywym silnikiem
xretractor query.rql --noanykey </dev/null >server.log 2>&1 &
pid=$!
i=0
until grep -qx "PID: $pid" "$LOCK" 2>/dev/null; do
  if ! kill -0 "$pid" 2>/dev/null || [ "$i" -ge 200 ]; then
    echo "serwer $pid nie przejal blokady $LOCK w ciagu 10 s"
    cat server.log
    exit 1
  fi
  sleep 0.05
  i=$((i + 1))
done
rm -f "$LOCK"
refused b b.out "$size_a"

# (c) podmiana: nowy plik pod ta sama sciezka, przez nikogo nietrzymany
: >"$LOCK"
refused c c.out "$size_a"

# (d) silnik zatrzymany - xtrdb znow pisze
stop_server
pid=""
rm -f "$LOCK"
xtrdb noprompt <append.script >d.out 2>&1 || {
  echo "(d) xtrdb odmawia po zatrzymaniu silnika"
  cat d.out
  exit 1
}
if [ "$(file_size guarded)" -le "$size_a" ]; then
  echo "(d) xtrdb nie dopisal rekordu po zatrzymaniu silnika"
  exit 1
fi
