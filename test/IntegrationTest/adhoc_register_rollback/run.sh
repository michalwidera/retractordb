#!/bin/bash
# Nieudany import ad-hoc ma zostawic plan w stanie sprzed komendy, a serwer ma liczyc dalej.
#
# getAdHoc wnosi wezly do ZYWEGO planu (importFrom + compile), a dopiero potem rejestruje je
# w modelu danych. Porazka po imporcie zostawiala w planie wezly bez instancji: nastepny slot
# przebudowywal tablice uchwytow (refreshStreamHandles), trafial na brak i konczyl proces.
# Klient dostawal blad, a chwile pozniej serwera juz nie bylo.
#
# Zadne znane RQL nie prowadzi do takiej porazki, wiec wymusza ja jednorazowy hak
# RDB_FAULT_ADHOC_REGISTER. Test sprawdza cztery rzeczy: odmowe z powodem w logu serwera,
# zycie serwera przez kolejne sloty, brak nowej nazwy w planie i - wada lustrzana - to, ze
# to samo zapytanie powtorzone bez awarii przechodzi i daje dane. Bez ostatniej kontroli
# naprawa "odmawiaj zawsze" tez bylaby zielona.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp

# Hak czyta wylacznie serwer; klient dostaje juz srodowisko bez niego.
export RDB_FAULT_ADHOC_REGISTER=1
server_start query.rql -k
unset RDB_FAULT_ADHOC_REGISTER

# Log silnika jest wspolny dla slotu puli i dopisywany - czytamy tylko linie od tej chwili.
SERVER_LOG="${TMPDIR:-/tmp}/xretractor.log"
log_mark=$(wc -l < "$SERVER_LOG" 2> /dev/null || echo 0)

ADHOC='SELECT src[0]+1 STREAM extra FROM src'

rc=0
xqry -a "$ADHOC" > out_fail.txt 2> err_fail.txt || rc=$?
if [ "$rc" -eq 0 ]; then
  echo "ad-hoc z wymuszona awaria zakonczyl sie sukcesem - hak nie zadzialal"
  exit 1
fi

tail -n +"$((log_mark + 1))" "$SERVER_LOG" > server_log_tail.txt
if ! grep -qF "RDB_FAULT_ADHOC_REGISTER" server_log_tail.txt; then
  echo "w logu serwera brak powodu odmowy; dopisane linie:"
  cat server_log_tail.txt
  exit 1
fi

# Sedno regresji: bez wycofania proces konczyl sie w NASTEPNYM slocie. Dwa wiersze dst to
# co najmniej dwa sloty po odmowie.
rc=0
xqry -s dst -m 2 > out_dst.txt 2> err_dst.txt || rc=$?
rows=$(grep -c '^[0-9]' out_dst.txt || true)
if [ "$rc" -ne 0 ] || [ "$rows" -ne 2 ] || ! kill -0 "$_server_pid" 2> /dev/null; then
  echo "serwer nie przezyl nieudanego importu ad-hoc: kod $rc, wierszy $rows"
  cat out_dst.txt err_dst.txt
  exit 1
fi

# Plan wrocil do stanu sprzed komendy: nowej nazwy w nim nie ma.
xqry -d > dir_after_fail.txt
if grep -qw extra dir_after_fail.txt; then
  echo "plan po nieudanym imporcie nadal zawiera 'extra':"
  cat dir_after_fail.txt
  exit 1
fi

# Wada lustrzana: to samo zapytanie, juz bez awarii, przechodzi i liczy.
rc=0
xqry -a "$ADHOC" > out_ok.txt 2> err_ok.txt || rc=$?
if [ "$rc" -ne 0 ]; then
  echo "powtorzone ad-hoc odrzucone (kod $rc) - wycofanie zostawilo plan, ktory go nie przyjmuje"
  cat out_ok.txt err_ok.txt
  exit 1
fi
rc=0
xqry -s extra -m 2 > out_extra.txt 2> err_extra.txt || rc=$?
rows=$(grep -c '^[0-9]' out_extra.txt || true)
if [ "$rc" -ne 0 ] || [ "$rows" -ne 2 ]; then
  echo "powtorzone ad-hoc nie daje danych: kod $rc, wierszy $rows (oczekiwano 0 i 2)"
  cat out_extra.txt err_extra.txt
  exit 1
fi

xqry -k > /dev/null
server_wait_exit
