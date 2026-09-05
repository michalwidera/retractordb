#!/bin/bash
# Routing automatyczny w xqry (etap 2c): klient sam znajduje wlasciciela strumienia.
#
# Sprawdzane jest szesc rzeczy, bo dopiero razem znacza "routing dziala":
#   1. --bus listuje obie instancje z ich strumieniami, BEZ kontaktu z serwerami,
#   2. -s <strumien> bez --server trafia do wlasciciela, i to do wlasciwego,
#   3. nieznana nazwa konczy sie "nie ma takiego strumienia", a NIE timeoutem,
#   4. komenda dotyczaca calej instancji (-k, -d) przy dwoch zywych zada --server,
#   5. ad-hoc przez granice serwera jest odrzucany i NIE zmienia planu zadnego z nich,
#   6. --bus nad OSIEROCONYM segmentem wraca natychmiast, a nie po budzecie klienta.
#
# Punkt (6) jest regresja na zasadzie projektowa etapu 2b/2c: wykrywanie instancji nie moze
# polegac na odpytywaniu serwerow z timeoutem, bo osierocony segment jest nieodroznialny od
# zywego az do wyczerpania budzetu (300 x 10 ms = 3 s), a przy N instancjach szukanie
# nieistniejacego strumienia kosztowaloby N x 3 s.
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
  # Stabilne pliki flock sa czescia protokolu; usuwamy pliki testowe dopiero po procesach.
  rm -f "$LOCK_A" "$LOCK_B"
  # Bramka higieny: zaden obiekt IPC ani testowy plik blokady tych instancji nie ma prawa zostac.
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

# Uruchamia komende, ktora MA sie nie udac, i zwraca jej kod wyjscia w $rc.
expect_failure() {
  set +e
  "$@" >expect_out.txt 2>&1
  rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    echo "komenda miala sie nie udac, a zwrocila 0: $*"
    cat expect_out.txt
    exit 1
  fi
}

xretractor alfa.rql --noanykey --name alfa </dev/null >alfa.log 2>&1 &
pid_a=$!
xretractor beta.rql --noanykey --name beta </dev/null >beta.log 2>&1 &
pid_b=$!

wait_for_lock "$LOCK_A" "$pid_a"
wait_for_lock "$LOCK_B" "$pid_b"

# (1) --bus: obie instancje ze swoimi strumieniami w czytelnej tabeli.
#
# Plik zapytan jest w slocie sciezka BEZWZGLEDNA, tak samo jak w pliku blokady: slot czyta
# operator (i przyszly wybor celu dostarczania w E3) z innego katalogu roboczego niz serwer,
# wiec pozycja wzgledna wskazywalaby u niego nieistniejacy plik. Prezentacja skraca ja do
# ostatniego katalogu i nazwy pliku; pelna wartosc pozostaje w magistrali.
# Kolumna MODE opisuje URUCHOMIENIE instancji: obie startuja bez zadnej opcji trybu, wiec
# obie sa "N". Legenda liter jest ostatnim wierszem tabeli.
xqry --bus > servers.txt
grep -qE "^alfa[[:space:]]+\|[[:space:]]+$pid_a[[:space:]]+\|[[:space:]]+N[[:space:]]*\|[[:space:]]+\.\.\./multiserver_routing/alfa\.rql[[:space:]]+\|.*\bdsta\b" servers.txt || {
  echo "--bus nie opisal instancji alfa:"; cat servers.txt; exit 1; }
grep -qE "^beta[[:space:]]+\|[[:space:]]+$pid_b[[:space:]]+\|[[:space:]]+N[[:space:]]*\|[[:space:]]+\.\.\./multiserver_routing/beta\.rql[[:space:]]+\|.*\bdstb\b" servers.txt || {
  echo "--bus nie opisal instancji beta:"; cat servers.txt; exit 1; }
grep -qE "^MODE: N=normal, R=realtime, .*S=service$" servers.txt || {
  echo "--bus nie wypisal legendy trybow:"; cat servers.txt; exit 1; }
[ "$(wc -l < servers.txt)" -eq 5 ] || {
  echo "--bus wypisal tabele o nieoczekiwanej liczbie wierszy:"; cat servers.txt; exit 1; }

# (2) -s bez --server trafia do wlasciciela.
#
# Sprawdzamy PRZYNALEZNOSC do zbioru, a nie konkretna trojke wartosci: zrodla sa czytane
# w petli, a klient dolacza w dowolnym jej miejscu. Zbiory obu serwerow sa rozlaczne, wiec
# przynaleznosc wystarcza jako dowod, ze zapytanie poszlo do wlasciwej instancji.
xqry -s dsta -m 3 > out_alfa.txt
xqry -s dstb -m 3 > out_beta.txt

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

assert_values out_alfa.txt '^(11|21|31|41|51|61)[[:space:]]*$' "routing/dsta"
assert_values out_beta.txt '^(107|108|109)[[:space:]]*$' "routing/dstb"

# (3) Nieznany strumien: kod "nie ma takiego strumienia" (ENOENT = 2), nie timeout (ETIMEDOUT).
expect_failure xqry -s nosuchstream -m 1
if [ "$rc" -ne 2 ]; then
  echo "nieznany strumien dal kod $rc, oczekiwano 2 (no such file or directory)"
  cat expect_out.txt
  exit 1
fi
grep -q 'nosuchstream' expect_out.txt || {
  echo "komunikat nie wskazuje nazwy strumienia:"; cat expect_out.txt; exit 1; }

# (4) Komendy dotyczace calej instancji zadaja --server, gdy zywe sa dwie.
for arg in -k -d; do
  expect_failure xqry "$arg"
  grep -q -- '--server' expect_out.txt || {
    echo "xqry $arg nie zazadal --server:"; cat expect_out.txt; exit 1; }
  grep -q 'alfa' expect_out.txt || {
    echo "xqry $arg nie wymienil kandydatow:"; cat expect_out.txt; exit 1; }
done

# (5) Ad-hoc przez granice serwera: odmowa, i to BEZ skutku ubocznego. getAdHoc modyfikuje
#     plan serwera, wiec sam fakt odmowy nie wystarcza -- plany musza zostac nietkniete.
xqry --server alfa -d > dir_alfa_before.txt
xqry --server beta -d > dir_beta_before.txt

expect_failure xqry -a 'SELECT dsta[0]+dstb[0] STREAM crossed FROM dsta,dstb'
grep -q 'crosses a server boundary' expect_out.txt || {
  echo "ad-hoc przez granice nie zostal tak nazwany:"; cat expect_out.txt; exit 1; }

xqry --server alfa -d > dir_alfa_after.txt
xqry --server beta -d > dir_beta_after.txt
diff dir_alfa_before.txt dir_alfa_after.txt || {
  echo "odrzucony ad-hoc zmienil plan instancji alfa"; exit 1; }
diff dir_beta_before.txt dir_beta_after.txt || {
  echo "odrzucony ad-hoc zmienil plan instancji beta"; exit 1; }

# Ad-hoc mieszczacy sie w jednej instancji przechodzi bez --server i trafia do wlasciciela.
xqry -a 'SELECT dsta[0]+1000 STREAM adhoc_a FROM dsta'
xqry --server alfa -d | grep -q 'adhoc_a' || {
  echo "ad-hoc nie trafil do instancji alfa"; exit 1; }
xqry --server beta -d | grep -q 'adhoc_a' && {
  echo "ad-hoc trafil takze do instancji beta"; exit 1; }

# Regula nie ma klauzuli FROM, a mimo to ma jednoznacznego adresata: wlasciciela strumienia
# spod ON. Bez tej sciezki routing nie mialby sie tu czego chwycic i zazadalby --server.
xqry -a 'RULE r_beta ON dstb WHEN dstb[0] > 0 DO DUMP -1 TO 1' || {
  echo "regula ad-hoc nie zostala skierowana do wlasciciela strumienia dstb"; exit 1; }

expect_failure xqry -a 'RULE r_none ON nosuchstream WHEN nosuchstream[0] > 0 DO DUMP -1 TO 1'
grep -q -- '--server' expect_out.txt || {
  echo "regula na nieznanym strumieniu nie zazadala --server:"; cat expect_out.txt; exit 1; }

xqry --server alfa -k
xqry --server beta -k
wait "$pid_a" 2>/dev/null || true
wait "$pid_b" 2>/dev/null || true
pid_a=""
pid_b=""

# (6) Osierocony segment nie moze kosztowac budzetu klienta. Po zamknieciu obu serwerow
#     segment /dev/shm/xrdbbus ZOSTAJE (nikt go nie kasuje, bo skasowanie zywego zerwaloby
#     magistrale pozostalym), a jego sloty sa martwe. --bus musi to rozpoznac przez
#     /proc, czyli natychmiast -- nie po 3 s odpytywania.
start_ns=$(date +%s%N)
xqry --bus > orphan.txt 2>orphan.err || true
elapsed_ms=$(( ($(date +%s%N) - start_ns) / 1000000 ))
if [ "$elapsed_ms" -ge 1000 ]; then
  echo "--bus nad osieroconym segmentem trwalo ${elapsed_ms} ms -- wykrywanie przez timeout?"
  exit 1
fi
if grep -qE '^(alfa|beta)[[:space:]]+\|' orphan.txt; then
  echo "--bus wypisal martwa instancje:"
  cat orphan.txt
  exit 1
fi
