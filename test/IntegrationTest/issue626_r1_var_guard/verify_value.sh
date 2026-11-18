#!/usr/bin/env bash
# Czesc WYNIKOWA (Val) straznika #626 - biegnie w KAZDEJ konfiguracji przelacznikow.
#
# R1 (RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES) przepisuje (A>2)#(B>1) na (A#B)>3. Interwal
# i origin zostaja, ale ogon spada z 2 do 0, a rekord n powstaje w slocie (n+1+W)*Delta.
# ?VAR czyta wartosc zatwierdzona na koncu poprzedniego slotu (#531, D1), wiec bez straznika
# ten sam rekord logiczny opt_* widzial inna wartosc niz ref_* (3, 4, 5, ... wobec 5, 6, 7, ...).
# Plan z VAR nie moze wiec zalezec od R1: kazda para ref_* / opt_* ma byc bajtowo rowna,
# razem z .meta (origin i liczba rekordow), i to przy R1 ON oraz OFF.
#
# Asercje ksztaltu w tym skrypcie mowia, ze R1 NIE zadzialal - sa prawdziwe takze w ablacji,
# wiec nie trzeba ich wydzielac. Asercja, ze R1 zadzialal w planie bez VAR, stoi osobno
# w verify_fires.sh.
set -eu
. "$(dirname "$0")/../portable.sh"

rm -f ./*.meta ./*.desc ./*.shadow ref_* opt_* out_compile.txt
seq 1 100 > source.txt

xretractor var.rql -c > out_compile.txt

# Przepisywane wezly zachowuja ogon postaci sprzed R1, a wspolny przeplot nie powstaje.
grep -F "opt_read(1/15)	tail=2	origin=3" out_compile.txt
grep -F "opt_plain(1/15)	tail=2	origin=3" out_compile.txt
if grep -F "STREAM_TIMEMOVE(3)" out_compile.txt; then exit 1; fi

# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu i ogon zostaja.
# W trybie -c litera -f znaczy 'fields' w wyjsciu DOT - do kompilacji jej NIE dodawac.
xretractor var.rql -r -k -f -u

for pair in read down hits_out; do
  ref="ref_$pair"
  opt="opt_$pair"
  # Prog przeciw zielonemu wynikowi na pustce: 100 rekordow zrodla daje ponad 20 rekordow.
  [ "$(record_count "$ref" 4)" -ge 20 ] || {
    echo "$ref za krotki: $(file_size "$ref") B"
    exit 1
  }
  # Bez rosnacej zmiennej przesuniecie slotu nie zmienia wartosci i test nie ma czego wykryc.
  first=$(od -An -v -td4 -N4 "$ref" | xargs)
  last=$(od -An -v -td4 -j $(($(file_size "$ref") - 4)) "$ref" | xargs)
  [ "$first" -lt "$last" ] || {
    echo "$ref nie rosnie: pierwszy=$first ostatni=$last"
    exit 1
  }
  cmp "$ref" "$opt"
  # Naglowek .meta to osiem bajtow zarezerwowanych - porownanie je pomija.
  cmp <(tail -c +9 "$ref.meta") <(tail -c +9 "$opt.meta")
done

echo OK
