#!/usr/bin/env bash
# Zrzuty planu dla planow kontrolnych, cztery profile — wejscie strukturalnego
# warunku bramki `no_materialization`.
#
# Tu compile-only jest na miejscu i nie jest obejsciem reguly: twierdzenie brzmi
# "w planie nie powstaje substrat nad zrodlem ZA", czyli dotyczy KSZTALTU PLANU,
# a nie tego, czy plan sie wykonuje. Wykonywalnosc tych samych planow jest
# dowiedziona osobno, licznikami z `run_main_rdb.sh` (niezerowy mianownik).
set -euo pipefail

cd "$(dirname "$0")"
HERE="$(pwd)"
CODE_REPO="${CODE_REPO:-/home/michal/github/retractordb}"
OUT="${OUT:-$HOME/k26v3_gates_plans}"

rm -rf "$OUT"
for profile in DEFAULT NO_R2_CANON NO_R1_FACTOR NO_R1_NO_R2; do
  binary="$CODE_REPO/build/K26v3-$profile/src/retractor/xretractor"
  [ -x "$binary" ] || { echo "BLAD: brak binarki profilu $profile" >&2; exit 2; }
  mkdir -p "$OUT/$profile"
  for control in F9_R2_controls F9_R1_controls F9_X_controls; do
    work="$(mktemp -d)"
    cp "$HERE/rql/$control.rql" "$work/"
    cp "$HERE"/data/main/*.txt "$work/"
    mkdir -p "$work/temp"
    ( cd "$work" && RDB_BENCH_PLAN=1 "$binary" "$control.rql" -c ) \
      >"$OUT/$profile/$control.plan" 2>"$OUT/$profile/$control.probe"
    rm -rf "$work"
    printf '  ok  %-16s %s\n' "$profile" "$control"
  done
done
echo "OK: zrzuty planow kontrolnych w $OUT"
