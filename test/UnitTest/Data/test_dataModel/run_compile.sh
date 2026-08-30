#!/bin/bash
# Zrzut planu dla schematu oprawy dataModel (jeden strumien na operator
# strumieniowy: #, wyrazenie, +, %, >, -, .max).
#
# Skrypt zamiast `add_test(COMMAND sh -c "...")`: makro add_test zdefiniowane w
# test/IntegrationTest_serial/CMakeLists.txt jest w CMake globalne i obejmuje
# rowniez ten katalog. Przekazuje argumenty przez `_add_test(${ARGV})`, a
# `${ARGV}` w makrze jest lista sklejona srednikami — ponowne rozwiniecie tnie
# argument po jego WEWNETRZNYCH srednikach. Wykonywalo sie samo `set -e`, wiec
# test byl zawsze zielony i nigdy nic nie sprawdzil.
set -e

BUILD_DIR="${1:?usage: run_compile.sh <CMAKE_BINARY_DIR>}"

rm -f core0.desc core1.desc str1.desc str2.desc str3.desc str4.desc
rm -f str1 str1.meta str1.shadow str2 str2.meta str2.shadow \
  str3 str3.meta str3.shadow str4 str4.meta str4.shadow
rm -f out.txt out_compile.txt

"${BUILD_DIR}/src/retractor/xretractor" ut_example_schema.rql -c > out_compile.txt
cmake -E compare_files pattern_compile.txt out_compile.txt
