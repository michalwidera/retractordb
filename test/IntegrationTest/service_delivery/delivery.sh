#!/bin/bash
# Dostarczenie planu do dzialajacej uslugi (sciezka E3): `xretractor plan.rql` bez zadnych
# przelacznikow nadpisuje plik zapytan uslugi i zleca jej restart.
#
# Scenariusz pilnuje KOLEJNOSCI dwoch rozstrzygniec, ktora do 2026-09-06 byla odwrotna:
#
#  1. Najpierw ustalana jest usluga docelowa, dopiero potem odsiew konfliktow. Restart
#     zastapi plan uslugi, wiec nazwy strumieni, ktore ona trzyma dzisiaj, konfliktem nie sa.
#     Odsiew wylaczal wtedy nazwe NOWEGO uruchomienia (pusta przy zwyklym wywolaniu) zamiast
#     nazwy uslugi, wiec `xretractor plan.rql` przeciw usludze serwujacej TEN SAM plan konczyl
#     sie kodem 16 i planu nie dostarczal. Z `--name service` defekt sie maskowal, bo tam obie
#     nazwy sa te same -- stad drugi przebieg tego samego planu z jawna nazwa.
#  2. Konflikt z instancja INNA niz docelowa nadal zatrzymuje dostarczenie PRZED szkoda:
#     plik zapytan uslugi zostaje nietkniety, a restartu nie ma. Bez tego punktu naprawa
#     punktu pierwszego moglaby wylaczyc odsiew w calosci.
#  3. Jawnie wskazana tozsamosc (--name inna niz nazwa uslugi, --autoname) wygrywa nad
#     dostarczeniem: powstaje OSOBNA instancja, a plan uslugi zostaje nietkniety. Do
#     2026-09-06 zadanie "uruchom instancje foo" wykonywalo "nadpisz plan cudzej uslugi
#     i zrestartuj ja", konczylo sie kodem 0, a instancji foo nie bylo nigdzie -- przy
#     --autoname proces wypisywal nawet nazwe instancji, ktorej nigdy nie utworzyl.
#  4. Ta sama droga przy KOLIZJI nazw z usluga jest odmowa: instancja stawiana OBOK uslugi
#     nie ma prawa dostac nazwy strumienia, ktora usluga trzyma, bo obie maja zyc naraz.
#
# Metadane systemd sa atrapa: proces uruchomiony z `--service` jest instancja uslugowa na
# magistrali, ale UNIT/SCOPE bierze z /proc/self/cgroup, wiec pod ctestem ich nie ma. Test
# dopisuje je do pliku blokady sam, a `systemctl` podstawia atrapa z PATH -- realnego restartu
# jednostki systemd nie ma i byc nie moze.
#
# Katalog dziala na tozsamosci globalnej maszyny (nazwa instancji `service`), stad
# IT_NO_NAMESPACE w CMakeLists.txt i RUN_SERIAL. Z tego samego powodu nie korzysta
# z ../serverlib.sh: tamta oprawa pilnuje jednej instancji na sciezce blokady przestrzeni nazw.
set -e

LOCK_DIR="${TMPDIR:-/tmp}"
SERVICE_LOCK="$LOCK_DIR/xretractor_service.service.lock"
OTHER_LOCK="$LOCK_DIR/xretractor_service.other.lock"
FOO_LOCK="$LOCK_DIR/xretractor_service.foo.lock"

pid_service=""
pid_other=""
pid_foo=""
pid_auto=""
auto_name=""

cleanup() {
  local status=$?
  trap - EXIT INT TERM

  xqry --server service -k >/dev/null 2>&1 || true
  xqry --server other -k >/dev/null 2>&1 || true
  xqry --server foo -k >/dev/null 2>&1 || true
  if [ -n "$auto_name" ]; then xqry --server "$auto_name" -k >/dev/null 2>&1 || true; fi
  for pid in "$pid_service" "$pid_other" "$pid_foo" "$pid_auto"; do
    [ -n "$pid" ] || continue
    local waited=0
    while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt 50 ]; do
      sleep 0.1
      waited=$((waited + 1))
    done
    kill -KILL "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
  done

  # Plik blokady uslugi zostaje po normalnym koncu procesu, a ten test dodatkowo nadpisal
  # jego tresc atrapa metadanych -- zostawiony zmylilby kazdy nastepny start na tej maszynie.
  rm -f "$SERVICE_LOCK" "$OTHER_LOCK" "$FOO_LOCK"
  if [ -n "$auto_name" ]; then rm -f "$LOCK_DIR/xretractor_service.$auto_name.lock"; fi

  if ls /dev/shm/*.service /dev/shm/*.other /dev/shm/*.foo >/dev/null 2>&1; then
    echo "higiena: zostaly obiekty IPC w /dev/shm:"
    ls /dev/shm | grep -E '\.(service|other|foo)$' || true
    status=1
  fi
  if [ -n "$auto_name" ] && ls /dev/shm/*."$auto_name" >/dev/null 2>&1; then
    echo "higiena: zostaly obiekty IPC instancji $auto_name"
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
  echo "instancja $pid nie przejala blokady $lock w ciagu 10 s"
  return 1
}

rm -rf ./temp && mkdir -p ./temp
mkdir -p fakebin
cp fake_systemctl.sh fakebin/systemctl
chmod +x fakebin/systemctl

# Instancja obca startuje PRZED usluga. Odwrotna kolejnosc jest niewykonalna: przy zywej
# usludze kazde `xretractor <plan>` jest dostarczeniem planu, a nie startem drugiej instancji.
xretractor other.rql --noanykey --name other </dev/null >other.log 2>&1 &
pid_other=$!
wait_for_lock "$OTHER_LOCK" "$pid_other" || { cat other.log; exit 1; }

xretractor plan.rql --service --noanykey </dev/null >service.log 2>&1 &
pid_service=$!
wait_for_lock "$SERVICE_LOCK" "$pid_service" || { cat service.log; exit 1; }

# Atrapa metadanych jednostki. PID zostaje prawdziwy: to on jest dowodem, ze usluga zyje.
printf 'PID: %s\nPPID: 1\nMODE: service\nUNIT: fake-xretractor.service\nSCOPE: user\nQUERYFILE: %s/service_target.rql\n' \
  "$pid_service" "$PWD" >"$SERVICE_LOCK"

# --- 1. Plan kolidujacy WYLACZNIE z usluga jest dostarczany, a nie odrzucany ---------------

printf 'STARY PLAN\n' >service_target.rql
rm -f systemctl_called

status=0
PATH="$PWD/fakebin:$PATH" SYSTEMCTL_MARKER="$PWD/systemctl_called" \
  xretractor plan.rql --noanykey </dev/null >delivery.log 2>&1 || status=$?
if [ "$status" -ne 0 ]; then
  echo "dostarczenie planu do uslugi zakonczylo sie kodem $status"
  cat delivery.log
  exit 1
fi
grep -q "sent to running service 'fake-xretractor.service'" delivery.log || {
  echo "dostarczenie nie zameldowalo jednostki docelowej:"
  cat delivery.log
  exit 1
}
diff plan.rql service_target.rql >/dev/null || {
  echo "plik zapytan uslugi nie dostal tresci planu:"
  cat service_target.rql
  exit 1
}
[ -f systemctl_called ] || { echo "dostarczenie nie zlecilo restartu jednostki"; exit 1; }

# Kontrola: ta sama droga z jawna nazwa uslugi dzialala takze przed naprawa. Rozjazd miedzy
# tym przebiegiem a poprzednim byl calym objawem defektu, wiec oba stoja w tescie razem.
printf 'STARY PLAN\n' >service_target.rql
rm -f systemctl_called
status=0
PATH="$PWD/fakebin:$PATH" SYSTEMCTL_MARKER="$PWD/systemctl_called" \
  xretractor plan.rql --noanykey --name service </dev/null >delivery_named.log 2>&1 || status=$?
if [ "$status" -ne 0 ]; then
  echo "dostarczenie z --name service zakonczylo sie kodem $status"
  cat delivery_named.log
  exit 1
fi
diff plan.rql service_target.rql >/dev/null || {
  echo "przebieg z --name service nie dostarczyl planu"
  exit 1
}

# --- 2. Konflikt z instancja INNA niz docelowa zatrzymuje dostarczenie przed szkoda --------

printf 'STARY PLAN\n' >service_target.rql
rm -f systemctl_called

status=0
PATH="$PWD/fakebin:$PATH" SYSTEMCTL_MARKER="$PWD/systemctl_called" \
  xretractor other.rql --noanykey </dev/null >conflict.log 2>&1 || status=$?
if [ "$status" -eq 0 ]; then
  echo "plan kolidujacy z instancja 'other' zostal dostarczony do uslugi"
  cat conflict.log
  exit 1
fi
grep -q "stream 'osrc' is already served by instance 'other' (pid $pid_other)" conflict.log || {
  echo "odmowa nie wskazuje wlasciciela kolidujacej nazwy:"
  cat conflict.log
  exit 1
}
grep -qx 'STARY PLAN' service_target.rql || {
  echo "odrzucony plan mimo to nadpisal plik zapytan uslugi:"
  cat service_target.rql
  exit 1
}
[ ! -f systemctl_called ] || { echo "odrzucony plan mimo to zlecil restart jednostki"; exit 1; }

# --- 3. Jawna nazwa wygrywa nad dostarczeniem: powstaje osobna instancja --------------------

# Plan `named.rql` ma nazwy rozlaczne i z usluga, i z instancja `other`, wiec jedynym powodem
# odmowy moglaby byc sama obecnosc uslugi.
printf 'STARY PLAN\n' >service_target.rql
rm -f systemctl_called

PATH="$PWD/fakebin:$PATH" SYSTEMCTL_MARKER="$PWD/systemctl_called" \
  xretractor named.rql --noanykey --name foo </dev/null >foo.log 2>&1 &
pid_foo=$!
wait_for_lock "$FOO_LOCK" "$pid_foo" || { echo "instancja 'foo' nie wstala obok uslugi"; cat foo.log; exit 1; }

xqry --server foo -d | grep -qw 'nsrc' || {
  echo "instancja 'foo' nie serwuje wlasnego planu"
  xqry --server foo -d
  exit 1
}
grep -qx 'STARY PLAN' service_target.rql || {
  echo "start instancji 'foo' nadpisal plik zapytan uslugi:"
  cat service_target.rql
  exit 1
}
[ ! -f systemctl_called ] || { echo "start instancji 'foo' zlecil restart uslugi"; exit 1; }

xqry --server foo -k
wait "$pid_foo" 2>/dev/null || true
pid_foo=""

# --autoname idzie ta sama droga. Osobny przebieg, bo do 2026-09-06 wypisywal wylosowana nazwe
# instancji i zaraz potem meldowal dostarczenie do uslugi -- dwa sprzeczne zdania na stdout.
printf 'STARY PLAN\n' >service_target.rql
rm -f systemctl_called

PATH="$PWD/fakebin:$PATH" SYSTEMCTL_MARKER="$PWD/systemctl_called" \
  xretractor named.rql --noanykey --autoname </dev/null >auto.log 2>&1 &
pid_auto=$!
i=0
while [ "$i" -lt 200 ]; do
  auto_name=$(sed -n 's/^Instance name: //p' auto.log | head -1)
  [ -n "$auto_name" ] && break
  kill -0 "$pid_auto" 2>/dev/null || break
  sleep 0.05
  i=$((i + 1))
done
[ -n "$auto_name" ] || { echo "--autoname nie wypisal nazwy instancji"; cat auto.log; exit 1; }
AUTO_LOCK="$LOCK_DIR/xretractor_service.$auto_name.lock"
wait_for_lock "$AUTO_LOCK" "$pid_auto" || {
  echo "instancja '$auto_name' zapowiedziana przez --autoname nie wstala"
  cat auto.log
  exit 1
}
if grep -q 'sent to running service' auto.log; then
  echo "--autoname mimo wypisanej nazwy dostarczyl plan do uslugi:"
  cat auto.log
  exit 1
fi
grep -qx 'STARY PLAN' service_target.rql || { echo "--autoname nadpisal plik zapytan uslugi"; exit 1; }
[ ! -f systemctl_called ] || { echo "--autoname zlecil restart uslugi"; exit 1; }

xqry --server "$auto_name" -k
wait "$pid_auto" 2>/dev/null || true
rm -f "$AUTO_LOCK"
pid_auto=""

# --- 4. Instancja obok uslugi nie moze przejac nazw, ktore usluga trzyma --------------------

# Tu wlasnie widac, ze wylaczenie z odsiewu idzie za instancja ZASTEPOWANA: skoro planu nie
# dostarczamy, usluga zostaje zywa, a `plan.rql` chce jej nazw -- to konflikt, nie podmiana.
printf 'STARY PLAN\n' >service_target.rql
rm -f systemctl_called

status=0
PATH="$PWD/fakebin:$PATH" SYSTEMCTL_MARKER="$PWD/systemctl_called" \
  xretractor plan.rql --noanykey --name foo </dev/null >foo_conflict.log 2>&1 || status=$?
if [ "$status" -eq 0 ]; then
  echo "instancja 'foo' przejela nazwy strumieni zywej uslugi"
  cat foo_conflict.log
  exit 1
fi
grep -q "stream 'src' is already served by instance 'service' (pid $pid_service)" foo_conflict.log || {
  echo "odmowa nie wskazuje uslugi jako wlasciciela kolidujacej nazwy:"
  cat foo_conflict.log
  exit 1
}
grep -qx 'STARY PLAN' service_target.rql || { echo "odrzucony start 'foo' nadpisal plik zapytan uslugi"; exit 1; }
[ ! -f systemctl_called ] || { echo "odrzucony start 'foo' zlecil restart uslugi"; exit 1; }
