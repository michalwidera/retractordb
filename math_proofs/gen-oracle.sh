#!/usr/bin/env bash
# Generuje test/UnitTest/proofOracle.hpp: tablice liczone definicjami z Profs oraz skroty
# SHA-256 plikow Profs/*.lean, z ktorych je policzono. Test proof_drift porownuje te skroty
# z biezacym stanem dowodow, wiec po kazdej zmianie w Profs/ trzeba uruchomic ten skrypt ponownie.
#
#   math_proofs/gen-oracle.sh
set -euo pipefail

cd "$(dirname "$0")"
./link-lake.sh
OUT="../test/UnitTest/proofOracle.hpp"

lake build --wfail Profs profs_oracle
TABLES="$(lake exe profs_oracle)"

{
  echo "// WYGENEROWANE przez math_proofs/gen-oracle.sh - nie edytowac recznie."
  echo "#pragma once"
  echo "// clang-format off"
  echo ""
  echo "struct ProofSource {"
  echo "  const char *file;"
  echo "  const char *sha256;"
  echo "};"
  echo ""
  echo "// Skroty plikow, z ktorych policzono tablice (sprawdza test/proof_drift.py)."
  echo "inline constexpr ProofSource kProofSources[] = {"
  for f in Profs/*.lean; do
    echo "    {\"$f\", \"$(sha256sum "$f" | cut -d' ' -f1)\"},"
  done
  echo "};"
  echo ""
  echo "$TABLES"
} >"$OUT"

echo "Zapisano $(realpath "$OUT")"
