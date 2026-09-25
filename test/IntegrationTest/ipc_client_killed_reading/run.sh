#!/bin/bash
# Smierc klienta w trakcie odbioru odpowiedzi IPC ma kosztowac najwyzej jego wlasna komende
# (X-01, #254).
#
# Do X-01 odpowiedzi lezaly w boost::container::map w pamieci dzielonej pod named_mutex.
# Klient zabity z muteksem w reku zostawial go zajetym na zawsze (named_mutex nie jest
# robust): watek komunikacyjny serwera stawal na nim przy nastepnej odpowiedzi i serwer
# nie przyjmowal juz zadnej komendy, takze `xqry -k`. Teraz odpowiedzi leza w stalych
# slotach bez zamka, a slot martwego klienta serwer odzyskuje, gdy zabraknie wolnego.
#
# Klient staje w stanie READING przez hak RDB_FAULT_CLIENT_READ_DELAY i dopiero wtedy
# dostaje SIGKILL - znacznik haka na stderr mowi, ze juz tam stoi, wiec nic nie zalezy
# od zegara. Zabitych jest kSlots + 1: pierwszych kSlots zajmuje wszystkie sloty, a
# klient numer kSlots + 1 dochodzi do READING WYLACZNIE przez slot odzyskany. Jego znacznik
# jest wiec kontrola dodatnia odzysku, niezalezna od poziomu logowania (Release wycina WARN).
set -e
. "$(dirname "$0")/../serverlib.sh"

# ipc::kResponseSlotCount (constants.hpp).
SLOTS=16
MARKER="RDB_FAULT_CLIENT_READ_DELAY"

rm -rf temp client_*.err client_final.err
mkdir -p temp
server_start query.rql -k

for i in $(seq 1 $((SLOTS + 1))); do
  RDB_FAULT_CLIENT_READ_DELAY=60000 xqry -l >/dev/null 2>"client_$i.err" &
  client=$!
  waited=0
  until grep -q "$MARKER" "client_$i.err"; do
    if ! kill -0 "$client" 2>/dev/null; then
      echo "klient $i skonczyl sie, zanim doszedl do READING:"
      cat "client_$i.err"
      exit 1
    fi
    if [ "$waited" -ge 200 ]; then
      echo "klient $i nie doszedl do READING w ciagu 10 s"
      [ "$i" -gt "$SLOTS" ] && echo "(to klient ponad liczbe slotow - odzysk slotu nie zadzialal)"
      kill -KILL "$client" 2>/dev/null || true
      exit 1
    fi
    sleep 0.05
    waited=$((waited + 1))
  done
  kill -KILL "$client"
  wait "$client" 2>/dev/null || true
done

# `xqry -l` nic nie wypisuje: konczy sie zerem wtedy i tylko wtedy, gdy odpowiedz to db.world
# (qry::hello), a kazdy inny wynik - takze brak odpowiedzi - daje kod niezerowy.
status=0
xqry -l 2>client_final.err || status=$?
if [ "$status" -ne 0 ]; then
  echo "po $((SLOTS + 1)) zabitych klientach serwer nie odpowiedzial 'db world' (kod $status):"
  cat client_final.err
  exit 1
fi

xqry -k >/dev/null
server_wait_exit
