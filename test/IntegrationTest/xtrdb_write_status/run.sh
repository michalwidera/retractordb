#!/bin/bash
# Jedna konwencja powodzenia dla `append` i `write n` w xtrdb (#269). (a) `append` na zwyklym
# magazynie melduje `stored`. (b) `write n` na istniejacym rekordzie melduje `stored` (przed
# poprawka: `error`). (c) `append` do zrodla deklarowanego tylko do odczytu melduje `error`, nie
# zmienia pliku i nie konczy xtrdb przez FatalError.
#
# Kazdy przypadek jest oceniany osobno, zeby czerwony przebieg pokazal wszystkie awarie.
#
# Uzycie: run.sh <xtrdb>
xtrdb="$1"
status=0
rm -f ./*.desc ./*.meta ./*.shadow plain data.txt data.orig out.txt

# run_xtrdb <wejscie printf> - wynik w out.txt, kod wyjscia w $rc.
run_xtrdb() {
  # shellcheck disable=SC2059 # wejscie jest formatem printf z \n
  printf "$1" | (
    ulimit -v 2000000
    timeout 20 "$xtrdb" noprompt
  ) >out.txt 2>&1
  rc=$?
}

fail() { # fail <opis> <komunikat>
  echo "$1: $2 (kod $rc):"
  cat out.txt
  status=1
}

expect_rc0() { # expect_rc0 <opis>
  if [ "$rc" -ne 0 ]; then fail "$1" "kod wyjscia $rc zamiast 0"; fi
}

expect_line() { # expect_line <opis> <linia dokladnie>
  if ! tr -d '\r' <out.txt | grep -qx -- "$2"; then fail "$1" "brak linii '$2'"; fi
}

# (a) append na zwyklym magazynie.
run_xtrdb 'open plain { INTEGER a }\nappend\nstatus\nquit\n'
expect_rc0 "append na zwyklym magazynie"
expect_line "append na zwyklym magazynie" "stored"

# (b) write n na istniejacym rekordzie (regresja wlasciwa).
run_xtrdb 'open plain\nwrite 0\nstatus\nquit\n'
expect_rc0 "write 0 na istniejacym rekordzie"
expect_line "write 0 na istniejacym rekordzie" "stored"

# (c) append do zrodla deklarowanego tylko do odczytu.
printf '1 2\n' >data.txt
cp data.txt data.orig
printf '{\tINTEGER a\n\tINTEGER b\n\tREF "data.txt"\n\tTYPE TEXTSOURCE\n}\n' >src.desc
run_xtrdb 'open src\nappend\nstatus\nsize\nquit\n'
expect_rc0 "append do zrodla tylko do odczytu"
expect_line "append do zrodla tylko do odczytu" "error"
expect_line "append do zrodla tylko do odczytu" "0 Record(s)"
if ! cmp -s data.txt data.orig; then fail "append do zrodla tylko do odczytu" "data.txt zmieniony"; fi

exit $status
