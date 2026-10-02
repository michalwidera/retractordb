#!/bin/bash
# Pytanie o strumien, ktorego serwer nie zna, ma skonczyc sie BLEDEM w odpowiedzi, a serwer
# ma dalej liczyc i odpowiadac (issue #252).
#
# Granica tego, co test widzi. Nazwa spoza planu nie dochodzi do modelu danych: handler 'show'
# odrzuca ja wczesniej, na wyszukaniu w planie (qTree::getQuery), i ta droga jest tu
# pilnowana. Sama zmiana z #252 - dataModel siega po strumien przez streamRuntime() zamiast
# qSet[] - jest przez IPC nieosiagalna: nazwa obecna w planie, a nieobecna w modelu powstaje
# tylko po nieudanym imporcie ad-hoc, a wtedy nastepny slot konczy proces juz na
# refreshStreamHandles(). Te zmiane pilnuje ut_dataModel
# (xschema.missingStream_is_reported_not_inserted).
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp

server_start query.rql -k

# Log silnika jest wspolny dla slotu puli i dopisywany, wiec czytamy tylko linie dopisane od
# tej chwili - inaczej odmowa zapisana przez sasiada z tego samego slotu dawalaby tu zielen.
SERVER_LOG="${TMPDIR:-/tmp}/xretractor.log"
log_mark=$(wc -l < "$SERVER_LOG" 2> /dev/null || echo 0)

rc=0
xqry -s no_such_stream -m 1 > out_missing.txt 2> err_missing.txt || rc=$?

# no_such_file_or_directory, czyli selectResult::streamNotFound. Liczba z nazwy stalej, bo
# errno rozni sie miedzy platformami.
expected_rc=$(errno_value ENOENT)
if [ "$rc" -ne "$expected_rc" ]; then
  echo "xqry zakonczyl sie kodem $rc, oczekiwano $expected_rc"
  cat out_missing.txt err_missing.txt
  exit 1
fi

# Werdykt klienta bierze sie z listy strumieni, wiec o tym, ze SERWER oddal blad na 'show',
# mowi dopiero jego log: odmowa wyszukania w planie i wyjatek zamieniony w error.response.
tail -n +"$((log_mark + 1))" "$SERVER_LOG" > server_log_tail.txt
for expected in "Missing - no_such_stream" "Command processor failure"; do
  if ! grep -qF "$expected" server_log_tail.txt; then
    echo "w logu serwera brak '$expected'; dopisane linie:"
    cat server_log_tail.txt
    exit 1
  fi
done

# Serwer zyje - i to oba watki: komunikacyjny przyjmuje subskrypcje, a przetwarzania dalej
# liczy sloty i wysyla wiersze.
if ! kill -0 "$_server_pid" 2> /dev/null; then
  echo "serwer nie przezyl pytania o nieznany strumien"
  exit 1
fi

rc=0
xqry -s dst -m 2 > out_dst.txt 2> err_dst.txt || rc=$?
rows=$(grep -c '^[0-9]' out_dst.txt || true)
if [ "$rc" -ne 0 ] || [ "$rows" -ne 2 ]; then
  echo "po bledzie serwer nie oddaje danych: kod $rc, wierszy $rows (oczekiwano 0 i 2)"
  cat out_dst.txt err_dst.txt
  exit 1
fi

xqry -k > /dev/null
server_wait_exit
