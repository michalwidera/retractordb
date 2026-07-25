#!/usr/bin/env bash
set -eu

xretractor_bin=$1
mode=$2
dedup=$3
share=$4
commutative=$5
factor=$6
probe=$7

build_info=$("$xretractor_bin" --optimizer-build-info)
expected_info=$(printf '%s\n' \
  "RDB_OPT_DEDUP_SUBSTRATES=$dedup" \
  "RDB_OPT_SHARE_EQUIVALENT_SELECTS=$share" \
  "RDB_OPT_COMMUTATIVE_ADD=$commutative" \
  "RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$factor" \
  "RDB_BENCH_PROBE=$probe")

if [ "$build_info" != "$expected_info" ]; then
  echo "optimizer build info mismatch"
  echo "expected:"
  echo "$expected_info"
  echo "actual:"
  echo "$build_info"
  exit 1
fi

if [ "$mode" = "build-info" ]; then
  echo OK
  exit 0
fi

rm -rf temp
rm -f ./*.desc out_compile.txt
mkdir -p temp

"$xretractor_bin" query.rql -c > out_compile.txt

stream_source() {
  awk -v stream="$1" '
    $0 ~ "^" stream "\\(" { in_stream = 1; next }
    in_stream && /^\t:- PUSH_STREAM/ {
      sub(/^\t:- PUSH_STREAM\(/, "")
      sub(/\)$/, "")
      print
      exit
    }
    in_stream && /^[^\t]/ { exit }
  ' out_compile.txt
}

if [ "$share" = "ON" ]; then
  same1_source=$(stream_source same1)
  same2_source=$(stream_source same2)
  [ "${same1_source#STREAM_SELECT_}" != "$same1_source" ]
  [ "$same1_source" = "$same2_source" ]

  if [ "$commutative" = "ON" ]; then
    [ "$(stream_source commuted)" = "$same1_source" ]
  else
    [ "$(stream_source commuted)" = "B" ]
  fi
else
  ! grep -F 'STREAM_SELECT_' out_compile.txt
  [ "$(stream_source same1)" = "A" ]
  [ "$(stream_source same2)" = "A" ]
  [ "$(stream_source commuted)" = "B" ]
fi

if [ "$dedup" = "ON" ]; then
  ! grep -F 'STREAM_ADD_DA_DB(' out_compile.txt
  grep -F ':- PUSH_STREAM(dedup_owner)' out_compile.txt
else
  grep -F 'STREAM_ADD_DA_DB(' out_compile.txt
  grep -F ':- PUSH_STREAM(STREAM_ADD_DA_DB)' out_compile.txt
fi

if [ "$factor" = "ON" ]; then
  grep -F 'STREAM_HASH_FA_FB(' out_compile.txt
  grep -F ':- STREAM_TIMEMOVE(3)' out_compile.txt
  ! grep -F 'STREAM_TIMEMOVE_FA(' out_compile.txt
  ! grep -F 'STREAM_TIMEMOVE_FB(' out_compile.txt
else
  ! grep -F 'STREAM_HASH_FA_FB(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_FA(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_FB(' out_compile.txt
fi

if [ "$mode" = "plan" ]; then
  echo OK
  exit 0
fi

"$xretractor_bin" query.rql -r -k -m 48

if [ "$mode" = "factor-semantic" ]; then
  cmp temp/factored temp/factor_reference
  cmp <(tail -c +9 temp/factored.meta) <(tail -c +9 temp/factor_reference.meta)
elif [ "$mode" = "dedup-exact-semantic" ]; then
  cmp temp/dedup_shifted temp/dedup_reference
  cmp <(tail -c +9 temp/dedup_shifted.meta) <(tail -c +9 temp/dedup_reference.meta)
elif [ "$mode" = "dedup-steady-semantic" ]; then
  steady_records() {
    od -An -v -w8 -td4 "$1" | awk '
      started || $1 != 0 || $2 != 0 {
        started = 1
        print $1, $2
      }
    ' | head -n 20
  }
  cmp <(steady_records temp/dedup_shifted) <(steady_records temp/dedup_reference)
else
  cmp temp/same1 temp/same2
  cmp temp/same1 temp/commuted

  cmp <(tail -c +9 temp/same1.meta) <(tail -c +9 temp/same2.meta)
  cmp <(tail -c +9 temp/same1.meta) <(tail -c +9 temp/commuted.meta)
fi

echo OK
