# cd build/Debug && ninja install && ctest -R it_field_ref_outside_from -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Odwolanie do pola strumienia, ktorego nie ma w FROM (`core1[0]` przy `FROM
# merged`, gdzie `merged` = `SELECT * FROM core0 + core1`).
#
# localizeFieldOffsets() znalo offsety wylacznie bezposrednich zrodel z FROM i
# zrodel osiagalnych przez substraty systemowe. Brakujacy klucz
# `offsetMap[q.id][schema]` operator[] wstawial z wartoscia 0, wiec `core1[0]`
# czytalo pozycje 0 rekordu `merged` (`core0.a`, BYTE jako INTEGER) - bez
# ostrzezenia. Decyzja 2026-09-14: odwolanie jest nielegalne, kompilator odrzuca
# je kanalem `Check result:`.
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
