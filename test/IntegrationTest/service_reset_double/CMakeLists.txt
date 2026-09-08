# Dwa resety zlozone w czasie nie moga pomieszac planu z rezerwacja magistrali.
# Pelne uzasadnienie, log padajacego serwera i rola haka
# RDB_FAULT_PLAN_SWAP_DELAY: run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)
