#!/bin/bash
# Zatrzymanie sygnalem (#284). handleSignal() wolno tylko zapisac dwa atomiki: numer sygnalu
# i iLoopLimitCnt. Do 2026-10-04 handler logowal przez SPDLOG_WARN, a ten bierze muteks sinka
# dziennika. Sygnal dostarczony watkowi, ktory sam byl w srodku logowania, zakleszczal handler
# na tym muteksie, licznik petli nigdy nie dostawal stop_now i proces schodzil dopiero na
# SIGKILL po TimeoutStopSec.
#
# Czesc A - sygnal z zewnatrz w kazdym trybie pracy: bezczynnym (bez planu), na bramce
# --xqrywait i w zwyklej petli slotow. Proces ma skonczyc sie SAM, kodem 0 (dotychczasowy
# kontrakt), a komunikat o sygnale ma trafic do dziennika - wypisuje go juz run(), nie handler.
# Ta czesc pilnuje, ze zatrzymanie dziala; samego zakleszczenia nie wykaze, bo `kill` trafia
# w okno logowania tylko losowo.
#
# Czesc B - to okno otwarte deterministycznie. Hak RDB_FAULT_SIGNAL_IN_LOG zglasza sygnal
# z wnetrza sinka dziennika, czyli w watku trzymajacym jego muteks. Handler, ktory loguje,
# zakleszcza sie tu przy kazdym uruchomieniu. Zastrzezenie: w Release SPDLOG_WARN jest
# wyciety progiem ERROR, wiec tam czesc B przechodzi takze ze starym handlerem - defekt
# wykazuje Debug i RelWithDebInfo.
#
# Argumenty: sciezka xretractor z drzewa buildu, CMAKE_BUILD_TYPE (pusty = nie Release).
set -e
. "$(dirname "$0")/../serverlib.sh"

PATH="$(dirname "$1"):$PATH"
BUILD_TYPE="$2"
LOG="${TMPDIR:-/tmp}/xretractor.log"

rm -rf temp
mkdir -p temp
{
  echo "STORAGE 'temp'"
  echo
  echo "DECLARE a INTEGER STREAM src, 1/8 TEXTFILE 'data.txt'"
  echo
  echo "SELECT src[0] STREAM dst FROM src"
} > query.rql

# 15 s to zapas na obciazona maszyne CI; zatrzymanie trwa normalnie ponizej 0,2 s.
wait_for_exit() {
  local pid="$1" left=150
  while [ "$left" -gt 0 ]; do
    kill -0 "$pid" 2>/dev/null || return 0
    sleep 0.1
    left=$((left - 1))
  done
  return 1
}

# expect_clean_exit <opis> - serwer _server_pid ma skonczyc sie sam i kodem 0.
expect_clean_exit() {
  if ! wait_for_exit "$_server_pid"; then
    echo "$1: serwer zyje 15 s po sygnale"
    exit 1
  fi
  local status
  status=$(server_wait_status)
  if [ "$status" -ne 0 ]; then
    echo "$1: serwer zakonczyl sie kodem $status, oczekiwano 0"
    exit 1
  fi
}

# expect_log_line <opis> <sygnal> - komunikat wypisany po powrocie z handlera.
expect_log_line() {
  [ "$BUILD_TYPE" = "Release" ] && return 0
  if ! grep -q "Received SIG$2, initiating shutdown" "$LOG"; then
    echo "$1: brak komunikatu 'Received SIG$2' w $LOG"
    exit 1
  fi
}

# --- Czesc A: sygnal z zewnatrz w trzech trybach ---
for sig in TERM INT HUP; do
  for mode in idle gate slots; do
    case "$mode" in
      idle) args=(-k) ;;
      gate) args=(query.rql -k -x) ;;
      slots) args=(query.rql -k) ;;
    esac
    rm -f "$LOG"
    server_start "${args[@]}"
    # Chwila na wejscie w petle trybu: bramke, petle slotow albo petle bezczynna.
    sleep 0.5
    kill -"$sig" "$_server_pid"
    expect_clean_exit "SIG$sig w trybie $mode"
    expect_log_line "SIG$sig w trybie $mode" "$sig"
  done
done

# --- Czesc B: sygnal w watku trzymajacym muteks dziennika ---
# Bez server_start: sygnal przychodzi przed petla, wiec proces moze zniknac, zanim oprawa
# zobaczy linie "PID:" w blokadzie.
for sig in TERM INT HUP; do
  rm -f "$LOG"
  signum=$(kill -l "$sig")
  RDB_FAULT_SIGNAL_IN_LOG="$signum" xretractor query.rql -k </dev/null &
  _server_pid=$!
  _server_started=$_server_pid
  expect_clean_exit "SIG$sig pod muteksem dziennika"
  expect_log_line "SIG$sig pod muteksem dziennika" "$sig"
done

# --- Czesc C: subskrybent dostaje znacznik konca pracy (#521, etap 9b) ---
# Kontrakt executorsm: zatrzymanie (SIGTERM, SIGINT, SIGHUP, komenda kill) i przyjety --reset konczy
# epoke rozeslaniem OUT_OF_BUSSINESS do subskrybentow. Klient z niewyczerpanym budzetem -m konczy sie
# na znaczniku kodem 0. Bez znacznika czekalby do limitu ciszy i konczyl sie bledem "went silent"
# (it_xqry_server_silent) - to odroznia oba zachowania kodem wyjscia, bez pomiaru czasu.
printf '[timing]\nquery_no_data_timeout_ms = 5000\n' > client.toml
{
  echo "STORAGE 'temp'"
  echo
  echo "DECLARE a INTEGER STREAM src2, 1/8 TEXTFILE 'data.txt'"
  echo
  echo "SELECT src2[0] STREAM dst2 FROM src2"
} > query2.rql

subscribe() {
  # Obciecie w powloce testu, nie w przekierowaniu: `> sub.out` przy `&` wykonuje dopiero proces
  # potomny, wiec petla nizej potrafila przeczytac sub.out poprzedniej iteracji, uznac 2 wiersze
  # i wyslac sygnal, zanim xqry przeszedl show/get/detail - klient konczyl sie wtedy kodem 10
  # (wyjatek IPC, kolejka serwera juz zniknela). Tak padl CircleCI job 1237 (2026-10-25, HUP).
  : > sub.out
  xqry -s dst -m 1000 --config client.toml > sub.out 2> sub.err &
  _sub_pid=$!
  local i=0
  until [ "$(wc -l < sub.out)" -ge 2 ]; do
    if [ "$i" -ge 100 ] || ! kill -0 "$_sub_pid" 2>/dev/null; then
      echo "$1: subskrybent nie dostal 2 wierszy w ciagu 10 s"
      cat sub.err
      exit 1
    fi
    sleep 0.1
    i=$((i + 1))
  done
}

expect_subscriber_released() {
  if ! wait_for_exit "$_sub_pid"; then
    echo "$1: subskrybent zyje 15 s po koncu epoki"
    kill "$_sub_pid" 2>/dev/null || true
    exit 1
  fi
  local status=0
  wait "$_sub_pid" || status=$?
  if [ "$status" -ne 0 ]; then
    echo "$1: subskrybent zakonczyl sie kodem $status zamiast 0 na znaczniku konca pracy"
    cat sub.err
    exit 1
  fi
}

for stop in TERM INT HUP kill; do
  server_start query.rql -k
  subscribe "$stop"
  if [ "$stop" = kill ]; then xqry -k > /dev/null; else kill -"$stop" "$_server_pid"; fi
  expect_subscriber_released "zatrzymanie $stop"
  expect_clean_exit "zatrzymanie $stop z subskrybentem"
done

server_start query.rql -k
subscribe reset
xqry --reset query2.rql > /dev/null
expect_subscriber_released "--reset"
kill -0 "$_server_pid"
xqry -k > /dev/null
expect_clean_exit "zatrzymanie po --reset"
