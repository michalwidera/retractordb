#!/bin/bash
# Issue 167: HASH lewostronnie rekurencyjny, substrat wspolny z out2, -m 30.
# faccmemory implementuje kolowy bufor — teza testu to BRAK crashu przy HASH
# z retPosValue > 0. Wzorce planu i przebiegu sa dodatkowym zabezpieczeniem.
#
# Skrypt zamiast `add_test(COMMAND sh -c "...")`: makro add_test z katalogu
# nadrzednego przekazuje argumenty przez `_add_test(${ARGV})`, a `${ARGV}` w
# makrze jest lista sklejona srednikami. Ponowne rozwiniecie tnie argument po
# jego WEWNETRZNYCH srednikach, wiec wykonywalo sie samo `set -e`: test byl
# zawsze zielony i nigdy nic nie sprawdzil.
set -e

BUILD_DIR="${1:?usage: run.sh <CMAKE_BINARY_DIR>}"
XTRDB="${BUILD_DIR}/src/rdb/xtrdb"

rm -f out1 out1.desc out2 out2.desc out3 out3.desc out4 out4.desc out5 out5.desc \
  STREAM_ADD_s1_s2.desc STREAM_ADD_STREAM_ADD_s1_s2_s3.desc \
  STREAM_HASH_s1_s2.desc STREAM_HASH_s2_s3.desc \
  s1.desc s2.desc s3.desc s4.desc ./*.meta ./*.shadow

xretractor -c query.rql > out_compile.txt
# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten sam.
# Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera -f
# znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
xretractor query.rql -m 30 -f
"$XTRDB" noprompt < term.script > out_run.txt

bash ../compare.sh --ignore-eol pattern_compile.txt out_compile.txt
bash ../compare.sh --ignore-eol pattern_run.txt out_run.txt
