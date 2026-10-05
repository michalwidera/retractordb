#!/bin/bash
# REF z wczytanego `.desc` a uprawnienia xtrdb (#278). Magazyn lezy w `store`, pliki wskazane przez
# REF w `outside`. (a) Zrodlo deklarowane spoza katalogu magazynu czyta sie jak dotad, a `quitdrop`
# usuwa sam `.desc`, nie zrodlo. (b) Magazyn zapisywalny przeniesiony przez REF poza katalog jest
# odmowa przy `open`, a plik nie powstaje. (c) Ten sam magazyn z katalogiem w `storage.ref_dirs`
# otwiera sie i zapisuje. (d) Niepoprawny `storage.ref_dirs` konczy start xtrdb bledem.
#
# Warstwa uzytkownika konfiguracji idzie przez XDG_CONFIG_HOME w katalogu testu, wiec
# konfiguracja osoby uruchamiajacej nie wplywa na wynik.
#
# Kazdy przypadek jest oceniany osobno, zeby czerwony przebieg pokazal wszystkie awarie.
#
# Uzycie: run.sh <xtrdb>
. "$(dirname "$0")/../portable.sh"
xtrdb="$1"
status=0
rm -rf store outside xdg out.txt
mkdir -p store outside
export XDG_CONFIG_HOME="$PWD/xdg"
config="$XDG_CONFIG_HOME/retractor/retractor.toml"

# run_xtrdb <wejscie printf> - wynik w out.txt, kod wyjscia w $rc.
run_xtrdb() {
  # shellcheck disable=SC2059 # wejscie jest formatem printf z \n
  printf "$1" | (
    # Darwin nie egzekwuje RLIMIT_AS i odrzuca `ulimit -v` bledem; tam straza zostaje sam limit czasu.
    ulimit -v 2000000 2>/dev/null || true
    run_timeout 20 "$xtrdb" noprompt
  ) >out.txt 2>&1
  rc=$?
}

fail() { # fail <opis> <komunikat>
  echo "$1: $2 (kod $rc):"
  cat out.txt
  status=1
}

expect_rc() { # expect_rc <opis> <kod>
  if [ "$rc" -ne "$2" ]; then fail "$1" "kod wyjscia $rc zamiast $2"; fi
}

expect_text() { # expect_text <opis> <fragment>
  if ! tr -d '\r' <out.txt | grep -qF -- "$2"; then fail "$1" "brak tekstu '$2'"; fi
}

# (a) Zrodlo deklarowane spoza katalogu magazynu.
printf '11\n12\n' >outside/source.txt
printf '{\tINTEGER a\n\tREF "outside/source.txt"\n\tTYPE TEXTSOURCE\n}\n' >store/src.desc
run_xtrdb 'storage store\nopen src\nrread 0\nprintt\nquitdrop\n'
expect_rc "odczyt zrodla spoza katalogu" 0
expect_text "odczyt zrodla spoza katalogu" "{ a:11 }"
if [ ! -f outside/source.txt ]; then fail "quitdrop przy zrodle spoza katalogu" "zrodlo usuniete"; fi
if [ -f store/src.desc ]; then fail "quitdrop przy zrodle spoza katalogu" "store/src.desc zostal"; fi

# (b) Zapisywalny magazyn przeniesiony przez REF poza katalog, bez storage.ref_dirs.
printf '{\tINTEGER a\n\tREF "outside/target.bin"\n\tTYPE POSIX\n}\n' >store/tgt.desc
run_xtrdb 'storage store\nopen tgt\nquit\n'
expect_rc "zapis poza katalog bez ref_dirs" 0
expect_text "zapis poza katalog bez ref_dirs" "outside the storage directory; add its directory to storage.ref_dirs"
if [ -e outside/target.bin ]; then fail "zapis poza katalog bez ref_dirs" "outside/target.bin powstal"; fi
if [ ! -f store/tgt.desc ]; then fail "zapis poza katalog bez ref_dirs" "store/tgt.desc usuniety"; fi

# (c) Ten sam magazyn z katalogiem `outside` dopuszczonym przez operatora.
mkdir -p "$(dirname "$config")"
printf '[storage]\nref_dirs = ["%s"]\n' "$PWD/outside" >"$config"
run_xtrdb 'storage store\nopen tgt\nappend\nsize\nquit\n'
expect_rc "zapis do katalogu z ref_dirs" 0
expect_text "zapis do katalogu z ref_dirs" "1 Record(s)"
if [ ! -f outside/target.bin ]; then fail "zapis do katalogu z ref_dirs" "brak outside/target.bin"; fi

# (d) Wpis wzgledny w ref_dirs: blad konfiguracji, xtrdb nie startuje.
printf '[storage]\nref_dirs = ["outside"]\n' >"$config"
run_xtrdb 'quit\n'
expect_rc "niepoprawny ref_dirs" 1
expect_text "niepoprawny ref_dirs" "Configuration error: storage.ref_dirs must be an array of absolute directory paths"

exit $status
