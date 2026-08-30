#!/bin/bash
# Odczyt artefaktow oprawy dataModel przez xtrdb wzgledem pattern.txt.
# Deskryptory pochodza z szablonow .tpl, zeby porownanie nie zalezalo od tego,
# ktory test wytworzyl je jako ostatni.
#
# Skrypt zamiast `add_test(COMMAND bash -c "...")` — powod jak w run_compile.sh.
set -e

BUILD_DIR="${1:?usage: run_compare.sh <CMAKE_BINARY_DIR>}"

rm -f str1 str1.meta str1.shadow str2 str2.meta str2.shadow \
  str3 str3.meta str3.shadow str4 str4.meta str4.shadow

for name in core0 core1 str1 str2 str3 str4; do
  cmake -E copy_if_different "${name}.desc.tpl" "${name}.desc"
done

"${BUILD_DIR}/src/rdb/xtrdb" noprompt < term.script > out.txt
cmake -E compare_files pattern.txt out.txt
