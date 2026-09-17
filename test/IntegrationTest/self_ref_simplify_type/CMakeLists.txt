# cd build/Debug && ninja install && ctest -R it_self_ref_simplify_type -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Odwolanie do WLASNEJ nazwy zapytania w wyrazeniu, ktore upraszcza R3 (`SELECT
# merged[3] + 1 + 2 ... STREAM merged FROM core0 + core1`).
#
# compiler::simplifyFieldExpressions() bralo typ `merged[3]` przez
# sourceFieldAt("merged", 3), czyli z WLASNEGO pola wyjsciowego `merged_3`
# (INTEGER), a nie ze slotu 3 rekordu FROM (`core1.d`, FLOAT). Reasocjacja
# stalych, dopuszczalna tylko dla typow dokladnych, przepisywala `(d+1)+2` na
# `d+3` - i przy d = 2^24 pole zapisywalo 16777220 zamiast 16777218, bez
# ostrzezenia i tylko przy RDB_OPT_SIMPLIFY_EXPRESSIONS=ON.
#
# DWA wpisy: asercja o ksztalcie planu i asercja o policzonej wartosci nie
# dziela jednego wpisu ctest - patrz regula przy issue202_hash_shift_e2e w
# CLAUDE.md. Oczekiwany ksztalt (brak przepisania) jest prawdziwy takze przy
# wylaczonym przebiegu, wiec zaden wpis nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-shape
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash shape.sh)

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash value.sh)

set_tests_properties(${TestID}-shape ${TestID}-value PROPERTIES TIMEOUT 60)
