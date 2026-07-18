#!/usr/bin/env bash
# Weryfikacja bitowa tozsamosci okreznej przeplotu/rozplotu.
# Wzorce wyprowadzone z definicji formalnych (nie z implementacji):
#   c  : z = 2/3 -> okres B,A,A; B[k]=1001+k, A[k]=1+k
#   a2 : rekord 0 = null-placeholder (Theta opozniona o slot), potem sa od a_0
#   b2 : sb od b_0 bez opoznienia
set -e
rm -f ./*.meta ./*.desc ./*.shadow c a2 b2
xretractor query.rql -r -k -m 48

dump() { od -An -v -tu4 "$1" | xargs; }

exp_c=$(for i in $(seq 0 34); do
  if [ $((i % 3)) -eq 0 ]; then echo $((1001 + i / 3)); else echo $((1 + 2 * i / 3)); fi
done | xargs)
exp_a2=$(seq 0 22 | xargs)
exp_b2=$(seq 1001 1011 | xargs)

[ "$(dump c)" = "$exp_c" ] || {
  echo "c mismatch: $(dump c)"
  exit 1
}
[ "$(dump a2)" = "$exp_a2" ] || {
  echo "a2 mismatch: $(dump a2)"
  exit 1
}
[ "$(dump b2)" = "$exp_b2" ] || {
  echo "b2 mismatch: $(dump b2)"
  exit 1
}
echo OK
