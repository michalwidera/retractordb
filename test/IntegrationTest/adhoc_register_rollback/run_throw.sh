#!/bin/bash
# Wyjatek PO imporcie do zywego planu: droga catch (...) w getAdHoc. Rdzen zwraca porazki
# statusem (#512), ale wyjatek biblioteki albo bad_alloc nadal moze wyleciec z importu,
# kompilacji lub rejestracji w modelu - i tez musi przywrocic plan oraz oddac roszczenia
# magistrali. Wymusza go hak RDB_FAULT_ADHOC_REGISTER=throw; droga statusowa jest w run.sh.
# Kontrole jak tam: odmowa z powodem, zycie serwera, brak nazwy w planie i na magistrali,
# a potem to samo zapytanie bez awarii przechodzi i daje dane.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp

export RDB_FAULT_ADHOC_REGISTER=throw
server_start query.rql -k
unset RDB_FAULT_ADHOC_REGISTER

# Log silnika jest wspolny dla slotu puli i dopisywany - czytamy tylko linie od tej chwili.
SERVER_LOG="${TMPDIR:-/tmp}/xretractor.log"
log_mark=$(wc -l < "$SERVER_LOG" 2> /dev/null || echo 0)

ADHOC='SELECT src[0]+1 STREAM extra FROM src'

rc=0
xqry -a "$ADHOC" > throw_fail_out.txt 2> throw_fail_err.txt || rc=$?
if [ "$rc" -eq 0 ]; then
  echo "ad-hoc z wymuszonym wyjatkiem zakonczyl sie sukcesem - hak nie zadzialal"
  exit 1
fi
# Powod ma dojsc do klienta (error.response), a nie zostac tylko w logu serwera.
if ! grep -qF "command processor failure: RDB_FAULT_ADHOC_REGISTER" throw_fail_err.txt; then
  echo "klient nie podal powodu odmowy; stderr:"
  cat throw_fail_err.txt
  exit 1
fi
tail -n +"$((log_mark + 1))" "$SERVER_LOG" > throw_server_log_tail.txt
if ! grep -qF "Command processor failure: RDB_FAULT_ADHOC_REGISTER" throw_server_log_tail.txt; then
  echo "w logu serwera brak wyjatku z haka; dopisane linie:"
  cat throw_server_log_tail.txt
  exit 1
fi

rc=0
xqry -s dst -m 2 > throw_dst.txt 2> throw_dst_err.txt || rc=$?
rows=$(grep -c '^[0-9]' throw_dst.txt || true)
if [ "$rc" -ne 0 ] || [ "$rows" -ne 2 ] || ! kill -0 "$_server_pid" 2> /dev/null; then
  echo "serwer nie przezyl wyjatku importu ad-hoc: kod $rc, wierszy $rows"
  cat throw_dst.txt throw_dst_err.txt
  exit 1
fi

xqry -d > throw_dir_after_fail.txt
bus_own > throw_bus_after_fail.txt
if grep -qw extra throw_dir_after_fail.txt || grep -qE '\|[[:space:]]+extra$' throw_bus_after_fail.txt; then
  echo "wyjatek importu zostawil 'extra' w planie lub magistrali"
  cat throw_dir_after_fail.txt throw_bus_after_fail.txt
  exit 1
fi

rc=0
xqry -a "$ADHOC" > throw_ok_out.txt 2> throw_ok_err.txt || rc=$?
bus_own > throw_bus_after_ok.txt
if [ "$rc" -ne 0 ] || ! grep -qE '\|[[:space:]]+extra$' throw_bus_after_ok.txt; then
  echo "powtorzone ad-hoc po wyjatku odrzucone albo nieogloszone (kod $rc)"
  cat throw_ok_out.txt throw_ok_err.txt throw_bus_after_ok.txt
  exit 1
fi
rc=0
xqry -s extra -m 2 > throw_extra.txt 2> throw_extra_err.txt || rc=$?
rows=$(grep -c '^[0-9]' throw_extra.txt || true)
if [ "$rc" -ne 0 ] || [ "$rows" -ne 2 ]; then
  echo "powtorzone ad-hoc po wyjatku nie daje danych: kod $rc, wierszy $rows"
  cat throw_extra.txt throw_extra_err.txt
  exit 1
fi

xqry -k > /dev/null
server_wait_exit
