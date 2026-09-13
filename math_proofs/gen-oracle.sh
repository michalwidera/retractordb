#!/usr/bin/env bash
# Generuje test/UnitTest/proofOracle.hpp: tablice liczone definicjami z Profs oraz skroty
# SHA-256 plikow, z ktorych je policzono: Profs/*.lean, generatora OracleMain.lean i przypiec
# Lean/Mathlib/Verso (lakefile.lean, lean-toolchain, lake-manifest.json). Test proof_drift
# porownuje te skroty z biezacym stanem, wiec po kazdej zmianie tych plikow - takze po
# `install-lean.sh --upgrade` - trzeba uruchomic ten skrypt ponownie.
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
  # Ta sama lista co SOURCES w test/proof_drift.py.
  for f in Profs/*.lean OracleMain.lean lakefile.lean lean-toolchain lake-manifest.json; do
    echo "    {\"$f\", \"$(sha256sum "$f" | cut -d' ' -f1)\"},"
  done
  echo "};"
  echo ""
  echo "$TABLES"
} >"$OUT"

echo "Zapisano $(realpath "$OUT")"
