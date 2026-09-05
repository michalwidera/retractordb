#!/bin/bash
# Bledne zapytanie ad-hoc nie ma prawa zakonczyc procesu serwera.
#
# Do 2026-09-05 listenery bledow ANTLR-a w RQLParser.cpp wolaly exit(EPERM). W kliencie
# byl to zwykly kod wyjscia, ale w SERWERZE — smierc calej instancji: `xqry -a "ml"`
# ubijalo xretractora razem z planem i wszystkimi klientami, 5/5 prob. Zadna literowka
# w opcjach klienta nie byla do tego potrzebna, wystarczylo bledne zapytanie.
#
# Test sprawdza trzy rzeczy, bo dopiero razem znacza "serwer przezyl":
#   1. bledne zapytanie konczy sie porazka KLIENTA, z komunikatem o bledzie parsowania,
#   2. proces serwera nadal zyje,
#   3. nastepne POPRAWNE zapytanie ad-hoc dziala i dokłada strumien do planu.
#
# Punkt (3) nie jest ozdoba: status parsowania byl przed ta zmiana zmienna plikowa, ktorej
# parserRQLString nie zerowal na wejsciu, wiec samo zdjecie exit() zatrulo by kazde kolejne
# zapytanie w tym procesie. Odpowiednik jednostkowy: xparser.parse_failure_does_not_poison_the_next_parse.
set -e
. "$(dirname "$0")/../serverlib.sh"

xretractor plan.rql -c
server_start plan.rql

# (1) Bledne zapytanie: klient MUSI wyjsc niezerowo i powiedziec, ze to blad parsowania.
# xqry pisze diagnostyke na stderr (issue_217), wiec zbieramy oba strumienie.
set +e
bad_out=$(xqry -a "ml" 2>&1)
bad_rc=$?
set -e
if [ "$bad_rc" -eq 0 ]; then
  echo "bledne zapytanie ad-hoc zostalo przyjete (kod 0): $bad_out"
  exit 1
fi
case "$bad_out" in
  *"Fail parse"*) ;;
  *)
    echo "klient nie zglosil bledu parsowania; dostal: $bad_out"
    exit 1
    ;;
esac

# (2) Serwer zyje. To jest wlasciwa teza tego testu.
if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer zginal po blednym zapytaniu ad-hoc"
  exit 1
fi

# (3) Kolejne poprawne zapytanie nadal dziala.
ok_out=$(xqry -a 'select a[0] stream adhocok from core0' 2>&1) || {
  echo "poprawne zapytanie ad-hoc odrzucone po blednym: $ok_out"
  exit 1
}

# Artefakty powstaja po powrocie z xqry -a; czekamy na nie zamiast na zegar.
for _ in $(seq 1 200); do
  if [ -s temp/adhocok ] && [ -s temp/adhocok.desc ]; then
    break
  fi
  sleep 0.05
done
if ! { [ -s temp/adhocok ] && [ -s temp/adhocok.desc ]; }; then
  echo "brak artefaktow strumienia adhocok; temp zawiera:"
  ls -la temp
  echo "odpowiedz xqry -a: $ok_out"
  exit 1
fi

xqry -k
server_wait_exit
