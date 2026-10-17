# Mapa kodow wyjscia B.3 (#446): scenariusze bledow z tabeli analizy B, ktorych
# zaden inny test integracyjny nie przypinal. Wykaz scenariuszy i powod kazdego:
# run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: launcher.xretractor.17~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
