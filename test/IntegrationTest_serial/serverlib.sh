# Wspolna oprawa testow uruchamiajacych serwer xretractor.
#
# Powod istnienia: dziewiec testow powtarzalo ten sam schemat synchronizacji
#
#   xretractor query.rql -k -x &
#   while [ ! -f "$LOCK" ]; do sleep 0.1; done   # start serwera
#   ...
#   while [ -f "$LOCK" ]; do sleep 0.1; done     # rzekomy koniec serwera
#
# ktory ma dwie wady, obie potwierdzone:
#
#  1. Istnienie pliku blokady NIE dowodzi, ze wstal NASZ serwer. Instancja
#     zostawiona przez wczesniejszy test daje ten sam sygnal, a klient trafia
#     wtedy do cudzego planu. Sprawdzamy PID zapisany w blokadzie.
#  2. Znikniecie pliku blokady NIE dowodzi, ze proces sie zakonczyl.
#     FlockServiceGuard kasuje plik w destruktorze (lockManager.cpp,
#     cleanupLockFile), a handler zarejestrowany przez std::atexit
#     (executorsm.cpp, cleanup) dolacza watek komunikacyjny i sprzata IPC
#     dopiero PO wyjsciu z main. Miedzy tymi zdarzeniami proces zyje bez
#     blokady. Czekamy wiec na zakonczenie procesu przez `wait`.
#
# Uzycie:
#   . "$(dirname "$0")/../serverlib.sh"
#   server_start query.rql -k -x
#   xqry -s dst -k -m 3 > out.txt
#   server_wait_exit

SERVER_LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
_server_pid=""     # zywy serwer tego testu ("" gdy juz zakonczony)
_server_started="" # PID startowany przez ten test, do kontroli higieny

# Bramka higieny: test nie ma prawa zostawic po sobie ani procesu, ani blokady.
# Zostawiony serwer trzyma blokade instancji i wywraca KAZDY nastepny test, ktory
# probuje wystartowac silnik — awaria ujawnia sie wtedy u niewinnej ofiary,
# daleko od przyczyny. Sprawdzenie jest per-test wlasnie po to, zeby wskazywalo
# winowajce.
server_cleanup() {
  local status=$?
  trap - EXIT INT TERM

  if [ -n "$_server_pid" ] && kill -0 "$_server_pid" 2>/dev/null; then
    xqry -k >/dev/null 2>&1 || true
    local waited=0
    while kill -0 "$_server_pid" 2>/dev/null && [ "$waited" -lt 50 ]; do
      sleep 0.1
      waited=$((waited + 1))
    done
    kill -KILL "$_server_pid" 2>/dev/null || true
    wait "$_server_pid" 2>/dev/null || true
  fi

  if [ -n "$_server_started" ] && kill -0 "$_server_started" 2>/dev/null; then
    echo "higiena: test zostawil zywy proces xretractor $_server_started"
    status=1
  fi
  if [ -n "$_server_started" ] && grep -qx "PID: $_server_started" "$SERVER_LOCK" 2>/dev/null; then
    echo "higiena: test zostawil blokade $SERVER_LOCK nalezaca do $_server_started"
    status=1
  fi

  exit "$status"
}
trap server_cleanup EXIT INT TERM

# server_start <argumenty xretractor> — startuje serwer i czeka, az przejmie blokade.
server_start() {
  xretractor "$@" </dev/null &
  _server_pid=$!
  _server_started=$_server_pid

  local i=0
  while [ "$i" -lt 200 ]; do
    if grep -qx "PID: $_server_pid" "$SERVER_LOCK" 2>/dev/null; then
      return 0
    fi
    kill -0 "$_server_pid" 2>/dev/null || break
    sleep 0.05
    i=$((i + 1))
  done

  echo "serwer $_server_pid nie przejal blokady $SERVER_LOCK w ciagu 10 s"
  if [ -f "$SERVER_LOCK" ]; then
    echo "blokade trzyma:"
    head -3 "$SERVER_LOCK"
  else
    echo "plik blokady nie istnieje — serwer nie wstal"
  fi
  return 1
}

# server_wait_status — czeka na koniec procesu serwera i WYPISUJE jego kod wyjscia.
#
# Rozni sie od server_wait_exit tym, ze kodu nie ocenia. Uzywaja jej testy, w ktorych
# zakonczenie serwera jest oczekiwanym WYNIKIEM, a nie awaria — na przyklad sprawdzajace,
# ze blad krytyczny konczy proces czysto (EXIT_FAILURE), a nie SIGABRT-em czy SIGSEGV.
server_wait_status() {
  local pid="$_server_pid"
  [ -n "$pid" ] || { echo 0; return 0; }
  _server_pid=""
  local status=0
  wait "$pid" || status=$?
  echo "$status"
}

# server_wait_exit — czeka na FAKTYCZNE zakonczenie procesu serwera.
server_wait_exit() {
  local pid="$_server_pid"
  [ -n "$pid" ] || return 0
  _server_pid=""
  local status=0
  wait "$pid" || status=$?
  # 0 = wyjscie normalne, 143 = SIGTERM ze sprzatania. Cokolwiek innego znaczy,
  # ze serwer sie wywrocil, i test ma o tym powiedziec.
  if [ "$status" -ne 0 ] && [ "$status" -ne 143 ]; then
    echo "serwer zakonczyl sie kodem $status"
    return 1
  fi
  return 0
}
