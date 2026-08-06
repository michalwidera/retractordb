#!/usr/bin/env bash
# Regresja epoki logicznej zapytań ad hoc. Późny pass-through musi zachować
# bieżący indeks źródła zarówno w złączeniu, jak i w oknie AGSE.
set -eu

rm -rf temp
mkdir -p temp

server_pid=""
cleanup() {
  xqry -k >/dev/null 2>&1 || true
  if [ -n "$server_pid" ] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

xretractor query.rql -m 45 > adhoc-server.out 2> adhoc-server.err &
server_pid=$!

lock_file="${TMPDIR:-/tmp}/xretractor_service.lock"
for _ in $(seq 1 100); do
  [ -f "$lock_file" ] && break
  sleep 0.02
done
[ -f "$lock_file" ] || {
  echo "ad hoc: serwer nie utworzyl blokady"
  exit 1
}

# Plan ma już rekordy o indeksach większych niż origin=2.
sleep 0.6
xqry -a 'SELECT * STREAM late FROM win'

# Ad-hoc DECLARE jest odrzucane — serwer musi odpowiedzieć błędem i przetrwać.
# Dołączona w locie deklaracja nie miałaby bazy indeksu logicznego; wcześniej
# zapytanie z operatorem na takim źródle zabijało cały proces (issue #227).
declare_out=$(xqry -a "DECLARE a INTEGER STREAM adhoc_src, 0.1 FILE 'datafile1.txt'" 2>&1) && {
  echo "ad hoc: DECLARE nie zostalo odrzucone"
  exit 1
}
case "$declare_out" in
  *"DECLARE not supported"*) ;;
  *)
    echo "ad hoc: nieoczekiwana odpowiedz na DECLARE: $declare_out"
    exit 1
    ;;
esac
sleep 0.4
xqry -a 'SELECT * STREAM late_pair FROM late+win'
sleep 0.4
xqry -a 'SELECT * STREAM late_window FROM late@(3,3)'
sleep 0.2
xqry -a 'SELECT * STREAM late_sub FROM late-0.2'
sleep 0.2
xqry -a 'SELECT * STREAM late_hash FROM late#win'
sleep 0.2
xqry -a 'SELECT * STREAM late_left FROM late_hash&0.1'
sleep 0.2
xqry -a 'SELECT * STREAM late_right FROM late_hash%0.1'

wait "$server_pid"
server_pid=""

[ -s temp/late_pair ] || {
  echo "ad hoc: late_pair nie zawiera rekordow"
  exit 1
}

# Każdy rekord ma dwa 3-polowe okna. Obie połowy muszą być identyczne:
# late[n] jest pass-through win[n], a nie bieżącą wartością podpisaną origin=2.
od -An -v -w24 -td4 temp/late_pair | awk '
  NF != 6 || $1 == 0 || $1 != $4 || $2 != $5 || $3 != $6 { exit 1 }
  END { if (NR == 0) exit 1 }
' || {
  echo "ad hoc: polaczono rekordy z roznych indeksow logicznych"
  od -An -v -w24 -td4 temp/late_pair
  exit 1
}

[ -s temp/late_window ] || {
  echo "ad hoc: late_window nie zawiera rekordow"
  exit 1
}

# late jest 3-polowym oknem. Kolejne polecenie bierze trzy spłaszczone pola
# kończące się na pierwszym polu bieżącego rekordu: [curr[0], prev[2], prev[1]].
# Wynik musi pozostać pełny zamiast all-NULL powstałego przez odjęcie statycznego
# origin zamiast runtime'owej bazy.
od -An -v -w12 -td4 temp/late_window | awk '
  NF != 3 || $1 == 0 || $2 == 0 || $3 == 0 || $1 != $2 + 3 || $1 != $3 + 2 { exit 1 }
  END { if (NR == 0) exit 1 }
' || {
  echo "ad hoc: AGSE uzyl niewlasciwej bazy indeksu logicznego"
  od -An -v -w12 -td4 temp/late_window
  exit 1
}

[ -s temp/late_sub ] || {
  echo "ad hoc: late_sub nie zawiera rekordow"
  exit 1
}

# Różnica o dwukrotnie wolniejszym takcie wybiera co drugi rekord late.
# Sprawdzamy zarówno zawartość okna, jak i krok między rekordami; rozpoczęcie
# indeksowania ponownie od zera dawałoby tu odczyt sprzed runtime'owej bazy.
od -An -v -w12 -td4 temp/late_sub | awk '
  NF != 3 || $1 == 0 || $1 != $2 + 1 || $2 != $3 + 1 { exit 1 }
  NR > 1 && $1 != previous + 2 { exit 1 }
  { previous = $1 }
  END { if (NR < 2) exit 1 }
' || {
  echo "ad hoc: SUB uzyl niewlasciwego indeksu logicznego"
  od -An -v -w12 -td4 temp/late_sub
  exit 1
}

[ -s temp/late_hash ] || {
  echo "ad hoc: late_hash nie zawiera rekordow"
  exit 1
}

# late i win mają ten sam takt i tę samą treść logiczną. Przeplot powtarza
# zatem każde kolejne okno dwa razy. Wymagamy obu faz: powtórzenia i przejścia
# do następnego indeksu, a nie tylko niezerowego pliku wynikowego.
od -An -v -w12 -td4 temp/late_hash | awk '
  NF != 3 || $1 == 0 || $1 != $2 + 1 || $2 != $3 + 1 { exit 1 }
  NR > 1 {
    delta = $1 - previous
    if (delta == 0) seen_same = 1
    else if (delta == 1) seen_next = 1
    else exit 1
  }
  { previous = $1 }
  END { if (NR < 3 || !seen_same || !seen_next) exit 1 }
' || {
  echo "ad hoc: HASH uzyl niewlasciwego indeksu logicznego"
  od -An -v -w12 -td4 temp/late_hash
  exit 1
}

check_dehash() {
  local stream=$1
  [ -s "temp/$stream" ] || {
    echo "ad hoc: $stream nie zawiera rekordow"
    return 1
  }

  # Rozplot przeplotu dwóch identycznych strumieni ma odzyskać kolejne pełne
  # okna. Osobne sprawdzenie obu gałęzi chroni różne odwzorowania Div i Mod.
  od -An -v -w12 -td4 "temp/$stream" | awk '
    NF != 3 || $1 == 0 || $1 != $2 + 1 || $2 != $3 + 1 { exit 1 }
    NR > 1 && $1 != previous + 1 { exit 1 }
    { previous = $1 }
    END { if (NR < 2) exit 1 }
  ' || {
    echo "ad hoc: $stream uzyl niewlasciwego indeksu logicznego"
    od -An -v -w12 -td4 "temp/$stream"
    return 1
  }
}

check_dehash late_left
check_dehash late_right

echo OK
