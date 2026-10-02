# make install && ctest -R xtrdb_open_missing -V

# cmake-format: off
# `open` pliku bez `.desc` czyta schemat ze stdin. Do 2026-09-27 ani ta petla, ani glowna petla
# polecen nie sprawdzaly konca wejscia: `open` bez schematu doklejal ostatni token bez konca
# (8 GB w 2 min, #334), a nastepne polecenia skryptu szly do schematu. Szczegoly w run.sh.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_BINARY_DIR}/src/rdb/xtrdb)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
