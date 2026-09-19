#!/bin/bash
# Warstwa przenosnosci powloki dla testow integracyjnych.
#
# Powod istnienia: userland GNU i userland BSD roznia sie w drobiazgach, ktore w
# tescie nie daja bledu, tylko PUSTY WYNIK. `stat -c %s` na BSD konczy sie bledem
# uzycia, podstawienie wychodzi puste, a asercja `test "$(...)" -gt 16` mowi
# "integer expression expected" - albo, gorzej, arytmetyka `$(( / 4 ))` wywraca
# skrypt w miejscu niezwiazanym z tym, co test sprawdza. Kazda funkcja ponizej
# zastepuje jeden taki drobiazg i jest napisana tak, zeby dawala IDENTYCZNY wynik
# na obu systemach.
#
# Uzycie: `. "$(dirname "$0")/../portable.sh"` na poczatku skryptu testu.
# Plik jest kopiowany do drzewa budowy razem z reszta test/ (patrz file(COPY)
# w test/CMakeLists.txt), wiec sciezka wzgledna jest ta sama co dla compare.sh.

# Rozmiar pliku w bajtach. Zastepuje `stat -c %s` (GNU) i `stat -f %z` (BSD)
# jednym wywolaniem, ktore jest w POSIX-ie. `tr -d ' '` jest konieczne: BSD `wc`
# wyrownuje liczbe spacjami, a `test "  16" -gt 8` nie jest liczba dla `test`.
# Plik nieistniejacy daje 0, dokladnie jak dotychczasowe `|| echo 0`.
file_size() {
  [ -f "$1" ] || { echo 0; return 0; }
  wc -c <"$1" | tr -d ' '
}

# Liczba rekordow o stalej dlugosci w pliku. Skraca najczestszy wzorzec w tym
# drzewie: $(( $(stat -c %s f) / N )).
record_count() {
  echo $(( $(file_size "$1") / $2 ))
}

# Odczyt tablicy wartosci z pliku binarnego - zamiennik `od -An -v -t<fmt>`.
#
# `od -tf4` NIE jest przenosne. GNU drukuje 9 cyfr znaczacych, BSD (macOS) tylko 7
# (`%14.7e`), wiec 16777218 wychodzi tam jako `1.677722e+07`, a to odczytane z powrotem
# jest 16777220. it_self_ref_simplify_synth widzial przez to blad zaokraglenia, ktorego w
# bazie NIE BYLO: zapisane bity byly poprawne (4b800001), gubil je sam DRUK. Wartosci
# wymagajace do 7 cyfr - 2.5, 5, 7.5 - przechodzily, wiec defekt siedzial cicho w jednym
# tescie. `-tf8` ma u BSD 14 cyfr i zadne z uzywanych oczekiwan tyle nie potrzebuje, ale
# rozroznianie tego przy kazdym odczycie byloby pulapka na nastepna wartosc.
#
# Czytamy wiec plik wprost i rozpakowujemy bajty w kolejnosci maszyny, bez posrednictwa
# `od`. 9 cyfr dla binary32 i 17 dla binary64 to najmniejsza liczba cyfr, ktora zawsze
# wraca na te sama wartosc - i dokladnie tyle drukuje GNU `od`, wiec na Linuksie wynik
# nie zmienia sie ani o znak.
#
# Uzycie: read_binary_values <plik> <f4|f8|d4|d8>  ->  wartosci w jednej linii, po spacji.
read_binary_values() {
  python3 - "$1" "$2" <<'RDB_PY'
import struct
import sys

path, fmt = sys.argv[1], sys.argv[2]

# Kody jak w `od -t`: f4/f8 to IEEE-754, d4/d8 to liczby calkowite ze znakiem.
layout = {"f4": ("f", 4, 9), "f8": ("d", 8, 17), "d4": ("i", 4, 0), "d8": ("q", 8, 0)}
if fmt not in layout:
    sys.exit("read_binary_values: nieobslugiwany format '%s'" % fmt)

code, width, digits = layout[fmt]

with open(path, "rb") as handle:
    data = handle.read()

if len(data) % width != 0:
    sys.exit("read_binary_values: rozmiar %s (%d B) nie jest wielokrotnoscia %d" % (path, len(data), width))

values = struct.unpack("=%d%s" % (len(data) // width, code), data)
print(" ".join(("%.*g" % (digits, v)) if digits else ("%d" % v) for v in values))
RDB_PY
}


# Znacznik czasu w nanosekundach. Zastepuje `date +%s%N`, ktorego BSD `date` nie
# zna: wypisuje wtedy literalne "N" na koncu, a nastepne $(( )) sie wywraca.
# python3 jest twarda zaleznoscia tego zestawu testow (patrz test/CMakeLists.txt),
# wiec nie dokladamy tu niczego nowego.
now_ns() {
  python3 -c 'import time; print(time.time_ns())'
}

# Uruchomienie z limitem czasu. `timeout` jest z GNU coreutils; macOS nie ma go
# wcale, a z Homebrew nazywa sie `gtimeout`. Ostatnia droga to wlasny straznik
# w tle - zachowuje sie jak `timeout` dla tego, czego uzywaja te testy: zabija
# proces po uplywie czasu i zwraca 124.
run_timeout() {
  local seconds="$1"
  shift
  if command -v timeout >/dev/null 2>&1; then
    timeout "$seconds" "$@"
    return $?
  fi
  if command -v gtimeout >/dev/null 2>&1; then
    gtimeout "$seconds" "$@"
    return $?
  fi
  # Polecenie idzie w tlo, ale ZE SWOIM WEJSCIEM. POSIX mowi, ze polecenie
  # asynchroniczne w powloce bez sterowania zadaniami dostaje stdin z /dev/null,
  # o ile nie ma JAWNEGO przekierowania - a wtedy `printf ... | run_timeout 10
  # xtrdb` czytalo pustke i test widzial zero rekordow zamiast bledu. Duplikat
  # pierwotnego wejscia na deskryptorze 9 i jawne `<&9` sa wlasnie tym wyjatkiem.
  exec 9<&0
  "$@" <&9 &
  local child=$!
  exec 9<&-
  # Straznik z ODCIETYM wyjsciem, i to nie jest kosmetyka. Podpowloka dziedziczy
  # deskryptory wolajacego, a gdy run_timeout stoi w potoku wewnatrz `$( ... )`,
  # jednym z nich jest KONIEC ZAPISU tego potoku. Podstawienie czeka, az zamkna go
  # WSZYSCY piszacy - wiec czekalo na `sleep` straznika takze wtedy, gdy polecenie
  # skonczylo sie natychmiast, i kazde wywolanie kosztowalo pelny limit czasu.
  # (Widoczne na macOS jako it_issue95_loopInCompile trwajacy rowno 10,04 s przy
  # limicie 10 s, a w tescie z kilkoma wywolaniami - jako przekroczenie limitu ctest.)
  (
    sleep "$seconds"
    kill -TERM "$child" 2>/dev/null
  ) >/dev/null 2>&1 &
  local guard=$!
  local status=0
  wait "$child" 2>/dev/null || status=$?
  kill -TERM "$guard" 2>/dev/null
  wait "$guard" 2>/dev/null || true
  # 143 = 128 + SIGTERM: proces zostal zabity przez straznika, czyli uplynal czas.
  [ "$status" -eq 143 ] && status=124
  return "$status"
}

# Korzen katalogow tymczasowych, BEZ konczacego ukosnika.
#
# Obcinanie ukosnika nie jest kosmetyka: na macOS $TMPDIR konczy sie nim zawsze
# (/var/folders/xx/.../T/), wiec "$TMPDIR/rdbtest.XXX" daje sciezke z podwojnym
# ukosnikiem w srodku. Powloka traktuje ja jak te sama sciezke, ale PORZADKUJE ja
# przy kazdym `cd`+`pwd` - i porownanie napisow "co wypisal program" z "co
# zlozyl test" przestaje pasowac, choc oba wskazuja jeden katalog.
rdb_tmp_root() {
  local base="${TMPDIR:-/tmp}"
  printf '%s' "${base%/}"
}

# Katalog tymczasowy. `mktemp -d` bez szablonu jest rozszerzeniem GNU; BSD
# `mktemp` wymaga szablonu i bez niego wypisuje uzycie na stderr, a podstawienie
# wychodzi puste.
make_temp_dir() {
  mktemp -d "$(rdb_tmp_root)/rdbtest.XXXXXXXX"
}

# Plik tymczasowy - z tego samego powodu co wyzej.
make_temp_file() {
  mktemp "$(rdb_tmp_root)/rdbtest.XXXXXXXX"
}

# Porownanie pierwszych N bajtow dwoch plikow. Zastepuje `cmp -n N a b`
# (rozszerzenie GNU; BSD `cmp` ma w tym miejscu zupelnie inna skladnie).
# `head -c` jest w obu userlandach.
cmp_prefix() {
  local bytes="$1" left="$2" right="$3"
  cmp <(head -c "$bytes" "$left") <(head -c "$bytes" "$right")
}

# Numeryczna wartosc stalej errno o podanej nazwie.
#
# Potrzebna, bo silnik zwraca kody bledow POSIX jako kod wyjscia procesu
# (boost::system::errc), a te sa ROZNE na roznych systemach: EPROTO to 71 na
# Linuksie i 100 na macOS, ENOSR - 63 i 98. Test, ktory wpisze liczbe na sztywno,
# sprawdza wiec jeden system, a na drugim melduje regresje, ktorej nie ma.
errno_value() {
  python3 -c 'import errno, sys; print(getattr(errno, sys.argv[1]))' "$1"
}

# Suma kontrolna SHA-256 samego skrotu, bez nazwy pliku. GNU ma sha256sum,
# BSD ma `shasum -a 256`.
sha256_of() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  else
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

sha512_of() {
  if command -v sha512sum >/dev/null 2>&1; then
    sha512sum "$1" | awk '{print $1}'
  else
    shasum -a 512 "$1" | awk '{print $1}'
  fi
}

# Czy plik blokady jest WOLNY (nikt nie trzyma na nim flocka). Zastepuje
# `flock -n <plik> true` z util-linux, ktorego macOS nie ma. Sam wywolanie
# systemowe flock(2) jest na obu systemach, wiec siegamy po nie przez python3.
# Zwraca 0, gdy blokada wolna; 1, gdy ktos ja trzyma.
lock_is_free() {
  python3 - "$1" <<'PY'
import fcntl, sys
try:
    handle = open(sys.argv[1], "a")
except OSError:
    sys.exit(0)  # pliku nie ma => nikt go nie trzyma
try:
    fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
except OSError:
    sys.exit(1)
sys.exit(0)
PY
}

# Gdzie leza obiekty IPC i czy w ogole daje sie je zobaczyc.
#
# Odpowiedz zalezy od tego, czy Boost.Interprocess ma do dyspozycji obiekty
# POSIX-owej pamieci dzielonej. Gdy ma (Linux), sa to wpisy shm_open widoczne
# jako pliki w /dev/shm. Gdy nie ma (Darwin deklaruje _POSIX_SHARED_MEMORY_OBJECTS
# jako wartosc UJEMNA, wiec Boost nawet nie probuje), sa to ZWYKLE PLIKI w jego
# katalogu roboczym pod /tmp.
#
# Rozstrzyga to plik shmEnv.sh generowany przez CMake z realnej wartosci makra
# Boosta - nie zgadujemy po nazwie systemu. Bez tego pliku (np. przy recznym
# uruchomieniu skryptu poza drzewem budowy) przyjmujemy wariant plikowy, bo na
# Linuksie i tak wygrywa warunek z /dev/shm.
if [ -f "${BASH_SOURCE[0]%/*}/shmEnv.sh" ]; then
  . "${BASH_SOURCE[0]%/*}/shmEnv.sh"
fi

shm_dir() {
  if [ -d /dev/shm ]; then
    echo /dev/shm
    return 0
  fi
  if [ "${RDB_SHM_POSIX:-0}" = "1" ]; then
    # Obiekty POSIX-owe bez /dev/shm nie maja w ogole reprezentacji w systemie
    # plikow - nie da sie ich wymienic ani policzyc. Kontrola higieny musi wtedy
    # zostac POMINIETA z komunikatem; cicho zdana jest gorsza niz zadna.
    return 2
  fi
  local root="${RDB_SHM_DIR_HINT:-/tmp}"
  local found
  found=$(find "$root" "${TMPDIR:-/tmp}" -maxdepth 2 -type d -name 'boost_interprocess' 2>/dev/null | head -1)
  if [ -n "$found" ]; then
    echo "$found"
    return 0
  fi
  # Katalogu jeszcze nie ma, bo nikt nie zalozyl ani jednego obiektu. To jest
  # ODPOWIEDZ ("nie ma zadnych obiektow"), a nie brak odpowiedzi: zwracamy
  # sciezke, ktora dopiero powstanie, a listowanie jej da pusty wynik.
  echo "$root/boost_interprocess"
  return 0
}

# Nazwy obiektow IPC pasujacych do wzorca ERE, po jednej w linii. Wzorzec dziala
# na SAMEJ NAZWIE obiektu, wiec "^..." i "...$" kotwicza tak, jak sie spodziewasz.
# Pusty wynik przy braku dopasowan; status 2, gdy katalogu obiektow nie ustalono.
shm_list() {
  local dir
  dir=$(shm_dir) || return 2
  [ -n "$dir" ] || return 2
  find "$dir" -maxdepth 2 -type f 2>/dev/null | sed 's|.*/||' | grep -E "$1" || true
}

# Usuniecie obiektow IPC pasujacych do wzorca ERE. Wzorzec dopasowuje SAMA NAZWE,
# dokladnie tak jak w shm_list - te dwie funkcje musza przyjmowac ten sam wzorzec,
# inaczej pierwszy uzytkownik napisze "^nazwa$" i cicho nic nie usunie.
# Milczy, gdy nie ma czego usuwac.
shm_remove() {
  local dir
  dir=$(shm_dir) || return 2
  [ -n "$dir" ] || return 2
  find "$dir" -maxdepth 2 -type f 2>/dev/null | while IFS= read -r victim; do
    if printf '%s' "${victim##*/}" | grep -qE "$1"; then rm -f "$victim"; fi
  done
  return 0
}
