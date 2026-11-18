#!/usr/bin/env bash
# Czesc KSZTALTU straznika #626: w planie bez VAR R1 nadal przepisuje (A>2)#(B>1) na
# (A#B)>3. Asercja mowi, ze przebieg ZADZIALAL, wiec przy
# RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=OFF jest tautologicznie falszywa i ten test jest
# tam wylaczony (DISABLED). Bez niego straznik wylaczajacy R1 zawsze przeszedlby
# verify_value.sh.
set -eu

rm -f ./*.meta ./*.desc out_fires.txt
seq 1 100 > source.txt

xretractor novar.rql -c > out_fires.txt

grep -F "opt_plain(1/15)	origin=3" out_fires.txt
grep -F ":- PUSH_STREAM(STREAM_HASH_A_B)" out_fires.txt
grep -F ":- STREAM_TIMEMOVE(3)" out_fires.txt
grep -F "STREAM_HASH_A_B(1/15)" out_fires.txt

echo OK
