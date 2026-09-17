# cd build/Debug && ninja install && ctest -R it_self_ref_field_shape -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Odwolanie do WLASNEJ nazwy zapytania (`SELECT merged[0], merged[2] STREAM
# merged FROM core0 + core1`) nad zrodlami o roznych typach pol.
#
# compiler::inferFieldShapes() bralo ksztalt `merged[k]` przez
# sourceFieldAt("merged", k), czyli z WLASNEGO schematu wyjsciowego `merged`, a
# nie z rekordu FROM. `merged_0` wychodzilo INTEGER zamiast BYTE, `merged_1`
# BYTE zamiast INTEGER - i 400 z `core1.c` zapisywalo sie jako 144, bez
# ostrzezenia.
#
# DWA wpisy: asercja o ksztalcie planu i asercja o policzonej wartosci nie
# dziela jednego wpisu ctest - patrz regula przy issue202_hash_shift_e2e w
# CLAUDE.md. Zaden nie zalezy od przelacznikow RDB_OPT_*, wiec zaden nie dostaje
# DISABLED pod ablacja.

add_test(
  NAME ${TestID}-shape
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash shape.sh)

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash value.sh)

set_tests_properties(${TestID}-shape ${TestID}-value PROPERTIES TIMEOUT 60)
