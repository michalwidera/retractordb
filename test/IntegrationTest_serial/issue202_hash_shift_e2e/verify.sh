#!/usr/bin/env bash
set -eu

rm -f ./*.meta ./*.desc matched CC out_compile.txt

xretractor query.rql -c > out_compile.txt

# `matched` must use one hash substrate and one accumulated shift (2 + 1).
grep -F 'matched(1/15)' out_compile.txt
grep -F ':- PUSH_STREAM(STREAM_HASH_A_B)' out_compile.txt
grep -F ':- STREAM_TIMEMOVE(3)' out_compile.txt
grep -F 'STREAM_HASH_A_B(1/15)' out_compile.txt

# `CC` spells the right-hand side out directly over independent inputs.
grep -F 'CC(1/15)' out_compile.txt
grep -F ':- PUSH_STREAM(STREAM_HASH_A2_B2)' out_compile.txt
grep -F 'STREAM_HASH_A2_B2(1/15)' out_compile.txt

# Neither side may retain per-input shift substrates.
if grep -F 'STREAM_TIMEMOVE_A(' out_compile.txt; then exit 1; fi
if grep -F 'STREAM_TIMEMOVE_B(' out_compile.txt; then exit 1; fi
if grep -F 'STREAM_TIMEMOVE_A2(' out_compile.txt; then exit 1; fi
if grep -F 'STREAM_TIMEMOVE_B2(' out_compile.txt; then exit 1; fi

xretractor query.rql -r -k -m 48

# Physical equality covers the complete stored INTEGER payload. Metadata headers
# contain per-stream creation timestamps, so compare the format-dependent body
# after that eight-byte field.
cmp matched CC
cmp <(tail -c +9 matched.meta) <(tail -c +9 CC.meta)

# Both sides must declare the same delay, in both of its components: interleave
# latency 2 (tail) plus shift 3 (origin). An equal payload with an unequal
# declaration would still be an unequal result.
#
# The split replaced a single tail=5. Since tau_N was restamped, the shift is no
# longer "not ready yet" (tail) but "this record has no definition" (origin);
# the sum, and therefore the emitted record sequence, is unchanged.
grep -F 'matched(1/15)	tail=2	origin=3' out_compile.txt
grep -F 'CC(1/15)	tail=2	origin=3' out_compile.txt

# Formula-derived payload for delta(A)=1/10 and delta(B)=1/5:
# A#B has the repeating order B,A,A. The equivalent shift of 2+1=3 output slots
# is carried by tail plus origin, so no placeholder records precede the data —
# the first stored record is interleave element 0: B[0],A[0],A[1],B[1],...
actual=$(od -An -v -td4 matched | xargs)
expected=$(
  record_count=$(($(stat -c %s matched) / 4))
  for element in $(seq 0 $((record_count - 1))); do
    cycle=$((element / 3))
    case $((element % 3)) in
      0) echo $((100 * (cycle + 1))) ;;
      1) echo $((2 * cycle + 1)) ;;
      2) echo $((2 * cycle + 2)) ;;
    esac
  done | xargs
)

[ "$actual" = "$expected" ] || {
  echo "matched payload mismatch"
  echo "expected: $expected"
  echo "actual:   $actual"
  exit 1
}

echo OK
