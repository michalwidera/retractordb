#!/usr/bin/env bash
set -eu

rm -rf temp
rm -f ./*.desc out_compile.txt
mkdir -p temp

xretractor query.rql -c > out_compile.txt

# Three positive equivalence classes have one computation substrate each.
grep -F 'STREAM_SELECT_c1(' out_compile.txt
grep -F 'STREAM_SELECT_e1(' out_compile.txt
grep -F 'STREAM_SELECT_x1(' out_compile.txt

grep -F 'c1(1/10)' out_compile.txt
grep -F 'c2(1/10)' out_compile.txt
grep -F 'e1(1/10)' out_compile.txt
grep -F 'e2(1/10)' out_compile.txt
grep -F 'x1(1/10)' out_compile.txt
grep -F 'x2(1/10)' out_compile.txt

# x2's private (q+p) substrate is dead after sharing; x3's differently grouped
# (r+q) substrate must remain.
if grep -F 'STREAM_ADD_q_p(' out_compile.txt; then exit 1; fi
grep -F 'STREAM_ADD_r_q(' out_compile.txt

# Order-sensitive queries stay as independent STREAM_ADD computations.
grep -F 'd1(1/10)' out_compile.txt
grep -F 'd2(1/10)' out_compile.txt
grep -F 'n1(1/10)' out_compile.txt
grep -F 'n2(1/10)' out_compile.txt

# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten sam.
# Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera -f
# znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
xretractor query.rql -r -k -m 18 -f

cmp temp/c1 temp/c2
cmp <(tail -c +9 temp/c1.meta) <(tail -c +9 temp/c2.meta)
cmp temp/e1 temp/e2
cmp <(tail -c +9 temp/e1.meta) <(tail -c +9 temp/e2.meta)
cmp temp/x1 temp/x2
cmp <(tail -c +9 temp/x1.meta) <(tail -c +9 temp/x2.meta)

# Public artifacts keep their own generated field names.
if cmp -s temp/c1.desc temp/c2.desc; then exit 1; fi
if cmp -s temp/e1.desc temp/e2.desc; then exit 1; fi

if cmp -s temp/d1 temp/d2; then exit 1; fi
if cmp -s temp/n1 temp/n2; then exit 1; fi
if cmp -s temp/x1 temp/x3; then exit 1; fi

echo OK
