# ctest -R xtrdb_write_status -V

# cmake-format: off
# Udany `write n` w xtrdb meldowal `error`, a `append` do zrodla deklarowanego tylko do odczytu
# konczyl xtrdb przez FatalError (#269). Szczegoly w run.sh.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_BINARY_DIR}/src/rdb/xtrdb)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
