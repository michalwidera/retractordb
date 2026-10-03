#!/bin/bash
# `open` bez `.desc` przyjmuje schemat wylacznie w nawiasach klamrowych; bez niego odmawia (#334).
#
# Kazdy przebieg idzie pod `ulimit -v` i limit czasu: przed poprawka xtrdb dochodzil do 8 GB w 2 min,
# wiec bez tych granic czerwony test zjadalby pamiec maszyny, zamiast pasc. Limit czasu daje
# run_timeout (portable.sh) - macOS nie ma `timeout` z GNU coreutils. `ulimit -v` macOS odrzuca
# ("cannot modify limit: Invalid argument"); tam zostaje sam limit czasu, ktory przy tamtym
# tempie wzrostu zatrzymuje proces przy okolo 1,3 GB.
#
# Uzycie: run.sh <xtrdb>
set -e
. "$(dirname "$0")/../portable.sh"
xtrdb="$1"
rm -f ./*.desc ./*.meta nosuch nosuch2 fresh fresh2 out.txt

# run_xtrdb <wejscie printf> - wynik w out.txt, kod wyjscia w $rc.
run_xtrdb() {
  set +e
  # shellcheck disable=SC2059 # wejscie jest formatem printf z \n
  printf "$1" | (
    ulimit -v 2000000 2>/dev/null || true
    run_timeout 20 "$xtrdb" noprompt
  ) >out.txt 2>&1
  rc=$?
  set -e
}

expect() { # expect <opis> <wzorzec w out.txt>
  if ! grep -qF -- "$2" out.txt; then
    echo "$1: brak '$2' w wyjsciu (kod $rc):"
    cat out.txt
    exit 1
  fi
}

expect_rc0() { # expect_rc0 <opis>
  if [ "$rc" -ne 0 ]; then
    echo "$1: kod $rc zamiast 0:"
    cat out.txt
    exit 1
  fi
}

no_file() { # no_file <opis> <plik>
  if [ -e "$2" ]; then
    echo "$1: odmowa zostawila plik $2"
    exit 1
  fi
}

# (1) Koniec wejscia zaraz po `open`: odmowa i zwykle wyjscie, bez zapetlenia na koncu wejscia.
run_xtrdb 'open nosuch\n'
expect_rc0 "open bez schematu, koniec wejscia"
expect "open bez schematu, koniec wejscia" "no descriptor file for 'nosuch'"
no_file "open bez schematu, koniec wejscia" nosuch.desc

# (2) Nastepne polecenie skryptu nie jest schematem: `descc` wykonuje sie na niepolaczonym xtrdb.
run_xtrdb 'open nosuch\ndescc\nquit\n'
expect_rc0 "open bez schematu, dalsze polecenia"
expect "open bez schematu, dalsze polecenia" "no descriptor file for 'nosuch'"
expect "open bez schematu, dalsze polecenia" "unconnected"
no_file "open bez schematu, dalsze polecenia" nosuch.desc

# (3) Schemat niedomkniety przed koncem wejscia: odmowa, bez pliku.
run_xtrdb 'open nosuch2 { INTEGER a\n'
expect_rc0 "schemat bez nawiasu zamykajacego"
expect "schemat bez nawiasu zamykajacego" "schema for 'nosuch2' is not closed with '}'"
no_file "schemat bez nawiasu zamykajacego" nosuch2.desc

# (4) Droga poprawna bez zmian: schemat w klamrach tworzy baze, a nastepne polecenie ja widzi.
run_xtrdb 'open fresh { INTEGER a STRING s[3] }\ndescc\nquit\n'
expect_rc0 "open ze schematem"
expect "open ze schematem" "{ INTEGER a STRING s[3] }"
if [ ! -s fresh.desc ]; then
  echo "open ze schematem nie utworzyl fresh.desc"
  cat out.txt
  exit 1
fi

# (5) Schemat sklejony z nawiasem (`{INTEGER`) tez jest schematem.
run_xtrdb 'open fresh2 {INTEGER a}\ndescc\nquit\n'
expect_rc0 "open ze schematem bez spacji"
expect "open ze schematem bez spacji" "{ INTEGER a }"
