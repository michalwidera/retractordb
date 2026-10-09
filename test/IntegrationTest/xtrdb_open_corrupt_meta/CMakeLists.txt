# make install && ctest -R xtrdb_open_corrupt_meta -V

# cmake-format: off
# `open` magazynu z uszkodzonym indeksem `.meta` (albo jego cieniem `.meta.shadow`) konczy sie
# odmowa i zwyklym wyjsciem xtrdb. Do 2026-10-09 bitsetSize spoza ramki wpisu rzucal z konstruktora
# metaData poza petle polecen i xtrdb konczyl sie std::terminate (kod 134, #421). Szczegoly w run.sh.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_BINARY_DIR}/src/rdb/xtrdb)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
