#!/usr/bin/env bash
# Odtwarza cały eksperyment: kontrole mutacyjne, macierz równoważności,
# most do silnika i podsumowanie. Jeden przebieg generuje każdą tabelę.
set -euo pipefail

cd "$(dirname "$0")"

XRETRACTOR=${XRETRACTOR:-../../../build/Debug/src/retractor/xretractor}
QUICK=${QUICK:-}

mkdir -p results/raw

echo "== 1/3 macierz równoważności =="
python3 test_equivalence.py ${QUICK:+--quick} --json results/equivalence.json \
  2>results/raw/equivalence.progress | tee results/raw/equivalence.txt

echo
echo "== 2/3 most do silnika =="
python3 engine_check.py --xretractor "$XRETRACTOR" --json results/engine.json \
  | tee results/raw/engine.txt

echo
echo "== 3/3 podsumowanie =="
python3 make_summary.py

echo
echo "gotowe: results/summary.md"
