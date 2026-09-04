#!/bin/bash
# Spojnosc odczytu: dwa strumienie (str1, str2) czytaja to samo zrodlo core0;
# str3 = str2 - str1 ma byc stale rowne 100. Kazda inna wartosc oznacza brak
# spojnosci odczytu z jednego pliku przez dwa strumienie. Sprawdzany jest takze
# zrzut kompilatora (-c) wzgledem pattern_compile.txt.
#
# Skrypt zamiast `add_test(COMMAND sh -c "...")`: makro add_test z katalogu
# nadrzednego przekazuje argumenty przez `_add_test(${ARGV})`, a `${ARGV}` w
# makrze jest lista sklejona srednikami. Ponowne rozwiniecie tnie argument po
# jego WEWNETRZNYCH srednikach, wiec polecenie rejestrowalo sie jako
# `sh "-c" "set -e " "rm ... " "xretractor ... "`. Powloka wykonywala samo
# `set -e` i konczyla zerem: test byl zawsze zielony i nigdy nic nie sprawdzil.
set -e

BUILD_DIR="${1:?usage: run.sh <CMAKE_BINARY_DIR>}"
XTRDB="${BUILD_DIR}/src/rdb/xtrdb"

rm -f ./*.meta

xretractor -c query-consitency.rql > out_compile.txt
# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten sam.
# Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera -f
# znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
xretractor query-consitency.rql -m 5 -f -v > verbose.txt
"$XTRDB" noprompt < term.script > out_xtrdb.txt

bash ../compare.sh --ignore-eol pattern_compile.txt out_compile.txt
bash ../compare.sh --ignore-eol pattern_xtrdb.txt out_xtrdb.txt
