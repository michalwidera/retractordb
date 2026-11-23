# cd build/Debug && ninja install && ctest -R it_horizon_xqry -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Horyzont osi czasu (#640, etap 2): zatrzymanie i restart na horyzoncie, raport
# `xqry --horizon` i odmowa zapytania ad hoc, ktorego horyzont juz minal.
# Scenariusz w run.sh.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
