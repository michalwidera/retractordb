# make install && ctest -R source_kinds -V

# cmake-format: off
# #346: jawne BINFILE, TEXTFILE i DEVICE w DECLARE - format zrodla ze slowa kluczowego, nie ze
# sciezki - oraz przestarzale `DECLARE ... FILE` tlumaczone zamrozona regula. Szczegoly w run.sh.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 180)

add_test(
  NAME ${TestID}-vg-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${IT_VALGRIND} xretractor kinds.rql -k -r -f -m 4)
