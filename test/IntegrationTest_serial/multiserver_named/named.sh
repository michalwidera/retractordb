#!/bin/bash
# Dwa serwery xretractor o roznych nazwach instancji pracuja rownoczesnie na jednej maszynie.
#
# Sprawdzane sa trzy rzeczy naraz, bo dopiero razem znacza "wieloserwerowosc dziala":
#   1. oba procesy zyja jednoczesnie (osobne pliki blokady, osobne obszary IPC),
#   2. kazdy serwuje SWOJE dane — wynik nie jest przypadkiem cudzym planem,
#   3. serwer nie zna strumienia drugiego serwera (DAG-i sa rozdzielone).
#
# Test nie korzysta z ../serverlib.sh: tamta oprawa pilnuje pojedynczej instancji na stalej
# sciezce blokady, czyli dokladnie tego zalozenia, ktore ten scenariusz znosi.
set -e

LOCK_DIR="${TMPDIR:-/tmp}"
LOCK_A="$LOCK_DIR/xretractor_service.alfa.lock"
LOCK_B="$LOCK_DIR/xretractor_service.beta.lock"

pid_a=""
pid_b=""

cleanup() {
  local status=$?
  trap - EXIT INT TERM
  xqry --server alfa -k >/dev/null 2>&1 || true
  xqry --server beta -k >/dev/null 2>&1 || true
  for pid in "$pid_a" "$pid_b"; do
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
  if ls /dev/shm/*alfa* /dev/shm/*beta* >/dev/null 2>&1; then
    echo "higiena: zostaly obiekty IPC w /dev/shm:"
    ls /dev/shm/ | grep -E 'alfa|beta' || true
    status=1
  fi
  if [ -f "$LOCK_A" ] || [ -f "$LOCK_B" ]; then
    echo "higiena: zostal plik blokady instancji"
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

xretractor alfa.rql --noanykey --name alfa </dev/null >alfa.log 2>&1 &
pid_a=$!
xretractor beta.rql --noanykey --name beta </dev/null >beta.log 2>&1 &
pid_b=$!

wait_for_lock "$LOCK_A" "$pid_a"
wait_for_lock "$LOCK_B" "$pid_b"

# (1) oba zyja rownoczesnie
kill -0 "$pid_a" 2>/dev/null || { echo "serwer alfa nie zyje"; cat alfa.log; exit 1; }
kill -0 "$pid_b" 2>/dev/null || { echo "serwer beta nie zyje"; cat beta.log; exit 1; }

# (2) kazdy serwuje swoj wlasny plan
xqry --server alfa -d > dir_alfa.txt
xqry --server beta -d > dir_beta.txt
grep -q 'dsta' dir_alfa.txt
grep -q 'dstb' dir_beta.txt
grep -q 'dstb' dir_alfa.txt && { echo "serwer alfa raportuje strumien bety"; exit 1; }
grep -q 'dsta' dir_beta.txt && { echo "serwer beta raportuje strumien alfy"; exit 1; }

# Dane: dsta = srca+1 (zbior {11,21,31,41,51,61}), dstb = srcb+100 (zbior {107,108,109}).
# Sprawdzamy PRZYNALEZNOSC do zbioru, a nie konkretna trojke wartosci: zrodla sa czytane
# w petli, a klient dolacza w dowolnym jej miejscu, wiec konkretna trojka jest kwestia
# momentu uruchomienia. Zbiory obu serwerow sa rozlaczne, wiec przynaleznosc wystarcza
# jako dowod, ze zaden z nich nie podal danych cudzego planu.
xqry --server alfa -s dsta -m 3 > out_alfa.txt
xqry --server beta -s dstb -m 3 > out_beta.txt

assert_values() {
  local file="$1" pattern="$2" label="$3"
  local values bad
  values=$(grep -v '^[[:space:]]*$' "$file")
  if [ -z "$values" ]; then
    echo "$label: serwer nie przyslal ani jednej wartosci"
    exit 1
  fi
  bad=$(printf '%s\n' "$values" | grep -vE "$pattern" || true)
  if [ -n "$bad" ]; then
    echo "$label: wartosci spoza oczekiwanego zbioru:"
    printf '%s\n' "$bad"
    exit 1
  fi
}

assert_values out_alfa.txt '^(11|21|31|41|51|61)[[:space:]]*$' "alfa/dsta"
assert_values out_beta.txt '^(107|108|109)[[:space:]]*$' "beta/dstb"

# (3) DAG-i rozdzielone: alfa nie zna strumienia bety
set +e
xqry --server alfa -s dstb -m 1 >cross.txt 2>&1
cross_rc=$?
set -e
if [ "$cross_rc" -eq 0 ]; then
  echo "serwer alfa obsluzyl strumien nalezacy do bety"
  cat cross.txt
  exit 1
fi

xqry --server alfa -k
xqry --server beta -k
wait "$pid_a" 2>/dev/null || true
wait "$pid_b" 2>/dev/null || true
pid_a=""
pid_b=""
