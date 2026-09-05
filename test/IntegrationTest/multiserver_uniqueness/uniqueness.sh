#!/bin/bash
# Magistrala xrdbbus egzekwuje rozlacznosc nazw strumieni miedzy instancjami xretractor.
#
# Sprawdzane jest jedenascie rzeczy, bo dopiero razem znacza "unikalnosc dziala i nie jest tepa":
#   1. druga instancja z kolidujaca nazwa strumienia ODMAWIA startu,
#   2. odmowa wskazuje wlasciciela: nazwe instancji i jej PID,
#   3. instancja o nazwach ROZLACZNYCH startuje normalnie (odmowa nie jest hurtowa),
#   4. po smierci wlasciciela (SIGKILL) nazwa jest znowu wolna -- martwy slot sprzata
#      ten, kto go zauwazy, bez demona i bez heartbeatow,
#   5. instancja bezimienna (tryb historyczny, bez --name) tez trzyma nazwy, a instancja
#      odprawiona z kwitkiem NIE kasuje jej obiektow IPC. Punkt piaty jest regresja na
#      konkretny defekt: odmowa nastepuje po std::atexit(cleanup), a cleanup kasuje obiekty
#      IPC WEDLUG nazwy instancji -- z nazwa jeszcze nieustawiona kasowalby nazwy historyczne,
#      czyli obiekty cudzego, dzialajacego serwera,
#   6. zapytanie AD-HOC powolujace nazwe nalezaca do innej instancji jest odrzucane, a plan
#      serwera nie rosnie o ta nazwe -- ad-hoc zmienia plan DZIALAJACEGO serwera, wiec
#      trafienie w cudza nazwe byloby trwalym skutkiem ubocznym, nie pomylka do powtorzenia,
#   7. nazwa powolana ad-hoc jest od tej chwili ROSZCZONA w magistrali: kolejna instancja
#      z ta sama nazwa dostaje odmowe wskazujaca serwer, ktory ja przyjal,
#   8. wspoldzielony plik licznika :ROTATION jest odmowa mimo ROZLACZNYCH nazw strumieni --
#      licznik nie jest nazwa strumienia, a dwie instancje na jednym pliku wczytuja te sama
#      wartosc i zapisuja te sama wartosc+1, czyli gubia rotacje i nadpisuja sobie archiwa.
#   9. druga instancja o TEJ SAMEJ nazwie odpada na flock przed skasowaniem artefaktow pierwszej,
#  10. dwa rownolegle starty z jedna nazwa strumienia daja dokladnie jednego wlasciciela, a
#      przegrany nie usuwa artefaktow zwyciezcy.
#  11. kolizja licznika jest wykrywana przed nadpisaniem pliku i restartem serwisu.
#
# Test nie korzysta z ../serverlib.sh: tamta oprawa pilnuje pojedynczej instancji na stalej
# sciezce blokady, czyli dokladnie tego zalozenia, ktore ten scenariusz znosi.
set -e

LOCK_DIR="${TMPDIR:-/tmp}"

pid_h=""
pid_a=""
pid_g=""
pid_d=""
pid_r=""
pid_race_a=""
pid_race_b=""
pid_service=""

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
  for name in gamma delta rota racea raceb; do
    xqry --server "$name" -k >/dev/null 2>&1 || true
  done
  for pid in "$pid_h" "$pid_a" "$pid_g" "$pid_d" "$pid_r" "$pid_race_a" "$pid_race_b" "$pid_service"; do
    [ -n "$pid" ] || continue
    local waited=0
    while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt 50 ]; do
      sleep 0.1
      waited=$((waited + 1))
    done
    kill -KILL "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
  done
  # Stabilne pliki flock pozostaja po normalnym koncu procesu. Test zna caly zbior swoich
  # nazw i usuwa je dopiero po zebraniu wszystkich dzieci.
  rm -f "$LOCK_DIR/xretractor_service.lock"
  for name in alfa beta gamma delta epsilon rota rotb racea raceb svc; do
    rm -f "$LOCK_DIR/xretractor_service.$name.lock"
  done
  # Bramka higieny: zaden obiekt IPC ani plik blokady tych instancji nie ma prawa zostac.
  if ls /dev/shm/*alfa* /dev/shm/*beta* /dev/shm/*gamma* /dev/shm/*delta* /dev/shm/*epsilon* /dev/shm/*rot?* \
      /dev/shm/*racea* /dev/shm/*raceb* \
      >/dev/null 2>&1; then
    echo "higiena: zostaly obiekty IPC w /dev/shm:"
    ls /dev/shm/ | grep -E 'alfa|beta|gamma|delta|epsilon|rota|rotb|racea|raceb' || true
    status=1
  fi
  for name in alfa beta gamma delta epsilon rota rotb racea raceb svc; do
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

# --- (9) ta sama nazwa instancji odpada przed skasowaniem artefaktow ----------------------

for artifact in dst dst.desc; do
  i=0
  while [ ! -f "$artifact" ] && [ "$i" -lt 100 ]; do
    sleep 0.05
    i=$((i + 1))
  done
  [ -f "$artifact" ] || { echo "alfa nie utworzyla artefaktu $artifact"; exit 1; }
done

set +e
xretractor alfa.rql --noanykey --name alfa </dev/null >same_name.log 2>&1
same_name_rc=$?
set -e
if [ "$same_name_rc" -eq 0 ]; then
  echo "druga alfa wystartowala mimo zajetej blokady instancji"
  exit 1
fi
for artifact in dst dst.desc; do
  [ -f "$artifact" ] || { echo "druga alfa skasowala artefakt $artifact pierwszej"; cat same_name.log; exit 1; }
done
xqry --server alfa -d | grep -q 'dst'

# --- (3) rozlaczne nazwy startuja normalnie ----------------------------------------------

xretractor gamma.rql --noanykey --name gamma </dev/null >gamma.log 2>&1 &
pid_g=$!
wait_for_lock "$LOCK_DIR/xretractor_service.gamma.lock" "$pid_g"

# --- (6) ad-hoc z cudza nazwa jest odrzucany i nie zostawia sladu w planie ----------------

# --server podany jawnie wygrywa nad routingiem: pytanie dotyczy tego, co robi gamma,
# a nie tego, dokad klient sam by trafil.
set +e
adhoc_out=$(xqry --server gamma -a "SELECT srcg[0]+9 STREAM dst FROM srcg" 2>&1)
adhoc_rc=$?
set -e

if [ "$adhoc_rc" -eq 0 ]; then
  echo "ad-hoc powolal strumien 'dst' nalezacy do alfy:"
  echo "$adhoc_out"
  exit 1
fi
case "$adhoc_out" in
  *"stream 'dst' is already served by instance 'alfa'"*) ;;
  *)
    echo "odmowa ad-hoc nie wskazuje wlasciciela kolidujacej nazwy:"
    echo "$adhoc_out"
    exit 1
    ;;
esac

# Sedno punktu: odmowa nastapila PRZED zmiana planu. 'dst' jako cale slowo, bo gamma
# serwuje 'dstg' -- dopasowanie podciagiem przepuscilaby tu kazdy wynik.
if xqry --server gamma -d | grep -qw 'dst'; then
  echo "odrzucony ad-hoc mimo to dolozyl 'dst' do planu gammy:"
  xqry --server gamma -d
  exit 1
fi

# --- (7) deklaracja przyjeta ad-hoc jest roszczona wobec pozostalych instancji ------------

# DECLARE nie ma klauzuli FROM, wiec przy wielu serwerach klient nie moze sam
# wybrac instancji. Odrzucenie musi nastapic przed zmiana ktoregokolwiek planu.
set +e
declare_auto_out=$(xqry -a "DECLARE vp INTEGER STREAM ambiguous_decl, 1 FILE 'data.txt'" 2>&1)
declare_auto_rc=$?
set -e
if [ "$declare_auto_rc" -eq 0 ]; then
  echo "DECLARE bez --server zostalo przyjete mimo wielu instancji"
  exit 1
fi
case "$declare_auto_out" in
  *"use --server"*) ;;
  *)
    echo "niejednoznaczny DECLARE nie wskazal --server:"
    echo "$declare_auto_out"
    exit 1
    ;;
esac

xqry --server gamma -a "DECLARE vp INTEGER STREAM adh, 1 FILE 'data.txt'"
xqry --server gamma -d | grep -qw 'adh' || {
  echo "gamma nie przyjela deklaracji ad-hoc 'adh'"
  xqry --server gamma -d
  exit 1
}

set +e
xretractor epsilon.rql --noanykey --name epsilon </dev/null >epsilon.log 2>&1
epsilon_rc=$?
set -e

if [ "$epsilon_rc" -eq 0 ]; then
  echo "serwer epsilon wystartowal mimo kolizji z nazwa powolana ad-hoc w gammie"
  cat epsilon.log
  exit 1
fi
grep -q "stream 'adh' is already served by instance 'gamma' (pid $pid_g)" epsilon.log || {
  echo "odmowa nie wskazuje gammy jako wlasciciela nazwy powolanej ad-hoc:"
  cat epsilon.log
  exit 1
}

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

# --- (8) wspoldzielony plik licznika :ROTATION jest odmowa ------------------------------

# Nazwy strumieni sa tu ROZLACZNE (dstr vs dsts) -- odmowa moze wiec wynikac wylacznie
# ze wspoldzielonego pliku licznika.
xretractor rota.rql --noanykey --name rota </dev/null >rota.log 2>&1 &
pid_r=$!
wait_for_lock "$LOCK_DIR/xretractor_service.rota.lock" "$pid_r"

set +e
xretractor rotb.rql --noanykey --name rotb </dev/null >rotb.log 2>&1
rotb_rc=$?
set -e

if [ "$rotb_rc" -eq 0 ]; then
  echo "serwer rotb wystartowal mimo wspoldzielonego pliku licznika"
  cat rotb.log
  exit 1
fi
grep -q "rotation counter file .*shared_counter.txt' is already used by instance 'rota' (pid $pid_r)" rotb.log || {
  echo "odmowa nie wskazuje wlasciciela pliku licznika:"
  cat rotb.log
  exit 1
}

# --- (11) ten sam konflikt zatrzymuje dostarczenie do serwisu przed szkoda ----------------

rm -f service_ready systemctl_called
printf 'STARY PLAN\n' >service_target.rql
mkdir -p fakebin
cp fake_systemctl.sh fakebin/systemctl
chmod +x fakebin/systemctl
(
  exec 9>"$LOCK_DIR/xretractor_service.svc.lock"
  flock -x 9
  printf 'PID: %s\nMODE: service\nUNIT: fake-xretractor.service\nSCOPE: user\nQUERYFILE: %s/service_target.rql\n' "$BASHPID" "$PWD" >&9
  : >service_ready
  while true; do sleep 1; done
) &
pid_service=$!
i=0
while [ ! -f service_ready ] && [ "$i" -lt 100 ]; do
  sleep 0.05
  i=$((i + 1))
done
[ -f service_ready ] || { echo "syntetyczny serwis nie przejal blokady"; exit 1; }

set +e
PATH="$PWD/fakebin:$PATH" SYSTEMCTL_MARKER="$PWD/systemctl_called" \
  xretractor rotb.rql --noanykey --name svc </dev/null >service_delivery.log 2>&1
service_delivery_rc=$?
set -e
if [ "$service_delivery_rc" -eq 0 ]; then
  echo "plan z kolizja licznika zostal dostarczony do serwisu"
  exit 1
fi
grep -qx 'STARY PLAN' service_target.rql || { echo "kolizja licznika nadpisala plik serwisu"; exit 1; }
[ ! -f systemctl_called ] || { echo "kolizja licznika uruchomila restart serwisu"; exit 1; }
grep -q "rotation counter file .*shared_counter.txt' is already used by instance 'rota'" service_delivery.log
kill "$pid_service"
wait "$pid_service" 2>/dev/null || true
pid_service=""

xqry --server rota -k
wait "$pid_r" 2>/dev/null || true
pid_r=""

xqry --server gamma -k
xqry --server delta -k
wait "$pid_g" 2>/dev/null || true
wait "$pid_d" 2>/dev/null || true
pid_g=""
pid_d=""

# --- (10) rownolegly start: roszczenie poprzedza kasowanie artefaktow ---------------------

xretractor racea.rql --noanykey --name racea </dev/null >racea.log 2>&1 &
pid_race_a=$!
xretractor raceb.rql --noanykey --name raceb </dev/null >raceb.log 2>&1 &
pid_race_b=$!

i=0
while [ "$i" -lt 200 ]; do
  alive=0
  kill -0 "$pid_race_a" 2>/dev/null && alive=$((alive + 1))
  kill -0 "$pid_race_b" 2>/dev/null && alive=$((alive + 1))
  [ "$alive" -eq 1 ] && break
  sleep 0.05
  i=$((i + 1))
done
if [ "$alive" -ne 1 ]; then
  echo "rownolegly start nie zostawil dokladnie jednego serwera"
  cat racea.log raceb.log
  exit 1
fi

if kill -0 "$pid_race_a" 2>/dev/null; then
  winner=racea
  winner_pid=$pid_race_a
  loser=raceb
  loser_pid=$pid_race_b
else
  winner=raceb
  winner_pid=$pid_race_b
  loser=racea
  loser_pid=$pid_race_a
fi
wait "$loser_pid" 2>/dev/null || true

# Sedno punktu: przegrany ma odpasc NA ROSZCZENIU, a nie pozniej. Sam brak procesu tego nie
# dowodzi -- odpadlby tak samo, gdyby zakonczyl sie po skasowaniu artefaktow zwyciezcy. Dowodem
# jest komunikat magistrali w jego logu.
grep -q "stream 'race' is already served by instance '$winner'" "$loser.log" || {
  echo "przegrany $loser nie odpadl na roszczeniu magistrali:"
  cat "$loser.log"
  exit 1
}

wait_for_lock "$LOCK_DIR/xretractor_service.$winner.lock" "$winner_pid"
xqry --server "$winner" -d | grep -q 'race'
# Komplet artefaktow zwyciezcy: dropArtifactFile kasuje trojke <id>, <id>.desc, <id>.meta, wiec
# regresja na przegranym, ktory doszedl do kasowania, ujawnia sie na kazdym z nich z osobna.
for artifact in race race.desc; do
  i=0
  while [ ! -f "$artifact" ] && [ "$i" -lt 100 ]; do
    sleep 0.05
    i=$((i + 1))
  done
  [ -f "$artifact" ] || { echo "brak artefaktu $artifact zwyciezcy $winner"; cat "$winner.log"; exit 1; }
done
xqry --server "$winner" -k
wait "$pid_race_a" 2>/dev/null || true
wait "$pid_race_b" 2>/dev/null || true
pid_race_a=""
pid_race_b=""
