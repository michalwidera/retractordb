#!/bin/bash
# Nieudany import ad-hoc ma zostawic plan w stanie sprzed komendy, a serwer ma liczyc dalej.
#
# getAdHoc wnosi wezly do ZYWEGO planu (importFrom + compile), a dopiero potem rejestruje je
# w modelu danych. Porazka po imporcie zostawiala w planie wezly bez instancji: nastepny slot
# przebudowywal tablice uchwytow (refreshStreamHandles), trafial na brak i konczyl proces.
# Klient dostawal blad, a chwile pozniej serwera juz nie bylo.
#
# Porazka po imporcie ma dwie drogi wycofania i test przechodzi obie. Wyjatek wymusza
# jednorazowy hak RDB_FAULT_ADHOC_REGISTER - tej drogi zadne znane RQL nie wywoluje. Status
# daje pozny blad open() magazynu: bramka RDB_FAULT_ADHOC_OPEN_GATE zatrzymuje serwer miedzy
# kontrola wstepna FILE a otwarciem pliku, a test w tym czasie podmienia katalog na zwykly plik.
# Dla obu drog test sprawdza odmowe z powodem, zycie serwera, brak nowej nazwy w planie i na
# magistrali (#303) oraz - wada lustrzana - to, ze to samo zapytanie powtorzone bez awarii
# przechodzi i daje dane. Bez ostatniej kontroli naprawa "odmawiaj zawsze" tez bylaby zielona.
# Na poczatku stoi jeszcze odmowa samej kontroli wstepnej, jeszcze przed roszczeniem nazw.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
rm -rf temp/late temp/late.desc
mkdir -p temp/late
rm -f open_gate.ready open_gate.release late_rc.txt

# Hak czyta wylacznie serwer; klient dostaje juz srodowisko bez niego.
export RDB_FAULT_ADHOC_REGISTER=1
export RDB_FAULT_ADHOC_OPEN_GATE="$PWD/open_gate"
export RDB_FAULT_ADHOC_OPEN_STREAM=late
server_start query.rql -k
unset RDB_FAULT_ADHOC_REGISTER
unset RDB_FAULT_ADHOC_OPEN_GATE RDB_FAULT_ADHOC_OPEN_STREAM

# Log silnika jest wspolny dla slotu puli i dopisywany - czytamy tylko linie od tej chwili.
SERVER_LOG="${TMPDIR:-/tmp}/xretractor.log"
log_mark=$(wc -l < "$SERVER_LOG" 2> /dev/null || echo 0)

# Otworzenie pliku SELECT w nieistniejacym katalogu nie moze ominac wycofania przez
# FatalError. Odmowa wypada przed roszczeniem nazw; plan, magistrala i serwer zostaja.
rc=0
xqry -a "SELECT src[0] STREAM blocked FROM src FILE 'missing/out'" > out_open.txt 2> err_open.txt || rc=$?
if [ "$rc" -eq 0 ] || ! grep -q "cannot open output file" err_open.txt; then
  echo "ad-hoc z nieotwieralnym FILE nie zwrocil odmowy z powodem"
  cat out_open.txt err_open.txt
  exit 1
fi
if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer zginal po odmowie otwarcia FILE"
  exit 1
fi
xqry -d > dir_after_open.txt
xqry --bus > bus_after_open.txt
if grep -qw blocked dir_after_open.txt || grep -qE '\|[[:space:]]+blocked$' bus_after_open.txt; then
  echo "nieudany import FILE zostawil strumien blocked w planie lub magistrali"
  cat dir_after_open.txt bus_after_open.txt
  exit 1
fi

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

# Magistrala tez wrocila do stanu sprzed komendy (#303). Roszczenie nazwy idzie PRZED importem,
# wiec bez zwolnienia `extra` zostawalo ogloszone jako strumien tej instancji: `xqry -s extra`
# trafial tu i dostawal "stream unknown", a inna instancja nie mogla tej nazwy zajac.
bus_own > bus_after_fail.txt
if grep -qE '\|[[:space:]]+extra$' bus_after_fail.txt; then
  echo "magistrala po nieudanym imporcie nadal oglasza 'extra':"
  cat bus_after_fail.txt
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
# Lustro kontroli magistrali: udany import ma nazwe oglosic. Bez tego zwolnienie "zawsze"
# przeszloby kontrole po porazce.
bus_own > bus_after_ok.txt
if ! grep -qE '\|[[:space:]]+extra$' bus_after_ok.txt; then
  echo "magistrala po udanym imporcie nie oglasza 'extra':"
  cat bus_after_ok.txt
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

# Kontrola wstepna widzi istniejacy katalog. Po sygnale .ready zamieniamy go
# w zwykly plik, wiec rzeczywiste open() zwraca ENOTDIR. Serwer ma odmowic
# bez FatalError i bez pozostawienia nazwy w planie lub magistrali.
(rc=0; xqry -a "SELECT src[0] STREAM late FROM src FILE 'late/out'" > late_out.txt 2> late_err.txt || rc=$?; echo "$rc" > late_rc.txt) &
late_client=$!
ready=0
for _ in $(seq 1 100); do
  if [ -f open_gate.ready ]; then ready=1; break; fi
  sleep 0.05
done
if [ "$ready" -ne 1 ]; then
  echo "bramka po kontroli FILE nie zostala osiagnieta"
  touch open_gate.release
  wait "$late_client" || true
  exit 1
fi
rmdir temp/late
: > temp/late
touch open_gate.release
wait "$late_client"
if [ "$(cat late_rc.txt)" -eq 0 ] || ! grep -q "cannot open output file" late_err.txt; then
  echo "pozny blad open() nie wrocil do klienta"
  cat late_out.txt late_err.txt
  exit 1
fi
if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer zginal po poznym bledzie open()"
  exit 1
fi
xqry -d > dir_after_late.txt
bus_own > bus_after_late.txt
if grep -qw late dir_after_late.txt || grep -qE '\|[[:space:]]+late$' bus_after_late.txt; then
  echo "pozny blad open() pozostawil nazwe late w planie lub magistrali"
  exit 1
fi
# .desc zapisany przed otwarciem pliku danych ma zniknac razem z odmowa; lustro po ponowieniu nizej.
if [ -e temp/late.desc ]; then
  echo "pozny blad open() zostawil temp/late.desc"
  exit 1
fi
rm temp/late
mkdir temp/late
xqry -a "SELECT src[0] STREAM late FROM src FILE 'late/out'" > late_retry_out.txt 2> late_retry_err.txt || {
  echo "ponowiony import po naprawie katalogu nie powiodl sie"
  cat late_retry_out.txt late_retry_err.txt
  exit 1
}
if [ ! -e temp/late.desc ]; then
  echo "udany import nie zapisal temp/late.desc - kontrola jego braku po odmowie niczego nie dowodzi"
  exit 1
fi
xqry -s late -m 2 > late_rows.txt
if [ "$(grep -c '^[0-9]' late_rows.txt || true)" -ne 2 ]; then
  echo "ponowiony import nie emituje danych"
  cat late_rows.txt
  exit 1
fi

xqry -k > /dev/null
server_wait_exit
