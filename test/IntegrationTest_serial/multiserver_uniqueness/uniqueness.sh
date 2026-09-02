#!/bin/bash
# Magistrala xrdbbus egzekwuje rozlacznosc nazw strumieni miedzy instancjami xretractor.
#
# Sprawdzane sa piec rzeczy, bo dopiero razem znacza "unikalnosc dziala i nie jest tepa":
#   1. druga instancja z kolidujaca nazwa strumienia ODMAWIA startu,
#   2. odmowa wskazuje wlasciciela: nazwe instancji i jej PID,
#   3. instancja o nazwach ROZLACZNYCH startuje normalnie (odmowa nie jest hurtowa),
#   4. po smierci wlasciciela (SIGKILL) nazwa jest znowu wolna -- martwy slot sprzata
#      ten, kto go zauwazy, bez demona i bez heartbeatow,
#   5. instancja bezimienna (tryb historyczny, bez --name) tez trzyma nazwy, a instancja
#      odprawiona z kwitkiem NIE kasuje jej obiektow IPC. Punkt piaty jest regresja na
#      konkretny defekt: odmowa nastepuje po std::atexit(cleanup), a cleanup kasuje obiekty
#      IPC WEDLUG nazwy instancji -- z nazwa jeszcze nieustawiona kasowalby nazwy historyczne,
#      czyli obiekty cudzego, dzialajacego serwera.
#
# Test nie korzysta z ../serverlib.sh: tamta oprawa pilnuje pojedynczej instancji na stalej
# sciezce blokady, czyli dokladnie tego zalozenia, ktore ten scenariusz znosi.
set -e

LOCK_DIR="${TMPDIR:-/tmp}"

pid_h=""
pid_a=""
pid_g=""
pid_d=""

# Kasuje slady instancji, ktora zginela od SIGKILL: po zabiciu procesu obiekty IPC zostaja
# w /dev/shm bezterminowo, a plik blokady zostaje (choc flock jest zwolniony). Sprzatamy je
# tutaj, bo to TEST je zabil -- bramka higieny nizej ma pilnowac serwerow zamknietych
# normalnie, a nie zaslaniac skutki wlasnego kill -KILL.
scrub_killed_instance() {
  local name="$1"
  rm -f /dev/shm/*."$name" /dev/shm/sem.*."$name" "$LOCK_DIR/xretractor_service.$name.lock"
}

cleanup() {
  local status=$?
  trap - EXIT INT TERM
  xqry -k >/dev/null 2>&1 || true
  for name in gamma delta; do
    xqry --server "$name" -k >/dev/null 2>&1 || true
  done
  for pid in "$pid_h" "$pid_a" "$pid_g" "$pid_d"; do
    [ -n "$pid" ] || continue
    local waited=0
    while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt 50 ]; do
      sleep 0.1
      waited=$((waited + 1))
    done
    kill -KILL "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
  done
  # Bramka higieny: zaden obiekt IPC ani plik blokady tych instancji nie ma prawa zostac.
  if ls /dev/shm/*alfa* /dev/shm/*beta* /dev/shm/*gamma* /dev/shm/*delta* >/dev/null 2>&1; then
    echo "higiena: zostaly obiekty IPC w /dev/shm:"
    ls /dev/shm/ | grep -E 'alfa|beta|gamma|delta' || true
    status=1
  fi
  for name in alfa beta gamma delta; do
    if [ -f "$LOCK_DIR/xretractor_service.$name.lock" ]; then
      echo "higiena: zostal plik blokady instancji $name"
      status=1
    fi
  done
  if [ -f "$LOCK_DIR/xretractor_service.lock" ]; then
    echo "higiena: zostal plik blokady instancji bezimiennej"
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

# --- (5) instancja bezimienna: kolizja wykryta, a jej obiekty IPC nietkniete ---------------

xretractor historic.rql --noanykey </dev/null >historic.log 2>&1 &
pid_h=$!
wait_for_lock "$LOCK_DIR/xretractor_service.lock" "$pid_h"

set +e
xretractor beta.rql --noanykey --name beta </dev/null >beta_vs_historic.log 2>&1
beta_rc=$?
set -e

if [ "$beta_rc" -eq 0 ]; then
  echo "serwer beta wystartowal mimo kolizji 'dst' z instancja bezimienna"
  cat beta_vs_historic.log
  exit 1
fi
grep -q "stream 'dst' is already served by the unnamed instance (pid $pid_h)" beta_vs_historic.log || {
  echo "odmowa nie wskazuje instancji bezimiennej jako wlasciciela:"
  cat beta_vs_historic.log
  exit 1
}

# Sedno regresji: odprawiona beta przeszla przez wlasny cleanup. Serwer bezimienny musi
# nadal odpowiadac -- gdyby beta skasowala jego segment i kolejke komend, to zapytanie
# wisialoby do wyczerpania budzetu klienta i skonczylo sie "server not found".
xqry -d | grep -q 'dst' || {
  echo "instancja bezimienna przestala odpowiadac po odmowie startu bety"
  cat historic.log
  exit 1
}

xqry -k
wait "$pid_h" 2>/dev/null || true
pid_h=""

# --- (1) i (2): kolizja nazwy strumienia jest odmowa wskazujaca wlasciciela ---------------

xretractor alfa.rql --noanykey --name alfa </dev/null >alfa.log 2>&1 &
pid_a=$!
wait_for_lock "$LOCK_DIR/xretractor_service.alfa.lock" "$pid_a"

set +e
xretractor beta.rql --noanykey --name beta </dev/null >beta.log 2>&1
beta_rc=$?
set -e

if [ "$beta_rc" -eq 0 ]; then
  echo "serwer beta wystartowal mimo kolizji nazwy strumienia 'dst'"
  cat beta.log
  exit 1
fi

grep -q "stream 'dst' is already served by instance 'alfa'" beta.log || {
  echo "odmowa nie wskazuje wlasciciela kolidujacej nazwy:"
  cat beta.log
  exit 1
}
grep -q "(pid $pid_a)" beta.log || {
  echo "odmowa nie podaje PID wlasciciela (oczekiwano $pid_a):"
  cat beta.log
  exit 1
}

# --- (3) rozlaczne nazwy startuja normalnie ----------------------------------------------

xretractor gamma.rql --noanykey --name gamma </dev/null >gamma.log 2>&1 &
pid_g=$!
wait_for_lock "$LOCK_DIR/xretractor_service.gamma.lock" "$pid_g"

# --- (4) po smierci wlasciciela nazwa jest znowu wolna ------------------------------------

kill -KILL "$pid_a"
wait "$pid_a" 2>/dev/null || true
pid_a=""
scrub_killed_instance alfa

xretractor delta.rql --noanykey --name delta </dev/null >delta.log 2>&1 &
pid_d=$!
wait_for_lock "$LOCK_DIR/xretractor_service.delta.lock" "$pid_d" || {
  echo "serwer delta nie przejal nazwy 'dst' po smierci alfy:"
  cat delta.log
  exit 1
}

# Delta faktycznie serwuje 'dst', a nie tylko wystartowala.
xqry --server delta -d | grep -q 'dst'

xqry --server gamma -k
xqry --server delta -k
wait "$pid_g" 2>/dev/null || true
wait "$pid_d" 2>/dev/null || true
pid_g=""
pid_d=""
