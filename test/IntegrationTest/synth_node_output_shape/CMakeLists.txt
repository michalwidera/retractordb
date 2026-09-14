# cd build/Debug && ninja install && ctest -R it_synth_node_output_shape -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Ksztalt wyjscia wezlow, w ktorych odwolanie do pola czyta slot rekordu
# wejsciowego o ksztalcie innym niz pole zrodla (2026-09-14):
#
# * B: jawna lista nad `@` i reduktorem zostawala przy INTEGER z parsera, bo
#   inferFieldShapes() pomijalo wezly syntetyzujace schemat. FLOAT 2.5 zapisywal
#   sie jako 2, srednia 3/2 jako 1.
# * C: buildOutputSchema() wpisywal reduktorowi RATIONAL na sztywno, a
#   descriptorFrom() liczy reductionResultField(). MAX nad DOUBLE 3000000000.5
#   przepelnial rational<int>.
# * D: `c2[k]` przy `FROM c2@(1,3)+kf` bralo typ pola k strumienia `c2` zamiast
#   slotu k bloku okna. 2.5*3 zapisywalo sie jako 7.
#
# DWA wpisy (regula przy issue202_hash_shift_e2e w CLAUDE.md). Zaden nie zalezy
# od RDB_OPT_*, wiec zaden nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-shape
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash shape.sh)

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash value.sh)

set_tests_properties(${TestID}-shape ${TestID}-value PROPERTIES TIMEOUT 60)
