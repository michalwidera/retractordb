#!/bin/bash
# `xqry -s -m N` przy serwerze, ktory zamilkl w trakcie odczytu, zanim klient dostal zamowione elementy.
# Serwer jest zamrazany SIGSTOP-em: zyje, trzyma blokade i kolejki, ale nic nie wysyla - tak z perspektywy
# klienta wyglada zawieszenie albo smierc bez pozegnania (brak rekordu konca strumienia).
#
# Do 2026-09-27 klient po limicie ciszy logowal "assuming server is dead" i konczyl sie ZEREM, jesli
# zdazyl dostac choc jeden wiersz. Skrypt testu, ktory ufal kodowi wyjscia, szedl wtedy dalej i padal
# dopiero na nastepnej komendzie - bez przyczyny w wyniku.
set -e
. "$(dirname "$0")/../serverlib.sh"
rm -rf temp && mkdir -p temp

server_start query.rql --noanykey

xqry -s dst -m 1000 --config short.toml >out.txt 2>err.txt &
client=$!
i=0
until [ "$(wc -l <out.txt)" -ge 3 ]; do
  if [ "$i" -ge 100 ]; then
    echo "klient nie dostal 3 wierszy w ciagu 10 s"
    cat err.txt
    exit 1
  fi
  sleep 0.1
  i=$((i + 1))
done

kill -STOP "$_server_pid"
status=0
wait "$client" || status=$?
kill -CONT "$_server_pid"

if [ "$status" -eq 0 ]; then
  echo "klient zakonczyl sie zerem, choc serwer zamilkl po $(wc -l <out.txt) z 1000 wierszy"
  cat err.txt
  exit 1
fi
grep -q 'dst: server did not answer within the timeout' err.txt || {
  echo "klient nie nazwal powodu na stderr"
  cat err.txt
  exit 1
}

xqry -k
server_wait_exit
