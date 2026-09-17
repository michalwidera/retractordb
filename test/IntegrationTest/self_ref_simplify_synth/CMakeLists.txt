# cd build/Debug && ninja install && ctest -R it_self_ref_simplify_synth -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# R3 nad wezlami, ktorych schemat SYNTETYZUJE operator FROM: okno `@` i reduktor
# (`FROM core1.max`).
#
# Rekord wejsciowy takiego wezla (query::descriptorFrom) ma sloty typu
# NAJSZERSZEGO z rekordu zrodla. compiler::simplifyFieldExpressions() bralo typ
# odwolania przez sourceFieldAt(): dla wlasnej nazwy z pola wyjsciowego, dla
# nazwy zrodla z pola `core1[k]` - w obu przypadkach INTEGER zamiast FLOAT.
# Reasocjacja stalych przepisywala `(x+1)+2` na `x+3` i przy x = 2^24 pole
# zapisywalo 16777220 zamiast 16777218, tylko przy
# RDB_OPT_SIMPLIFY_EXPRESSIONS=ON.
#
# DWA wpisy (regula przy issue202_hash_shift_e2e w CLAUDE.md). Oczekiwany
# ksztalt (brak przepisania) jest prawdziwy takze przy wylaczonym przebiegu,
# wiec zaden wpis nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-shape
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash shape.sh)

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash value.sh)

set_tests_properties(${TestID}-shape ${TestID}-value PROPERTIES TIMEOUT 60)
