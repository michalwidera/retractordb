# Dwa resety zlozone w czasie nie moga pomieszac planu z rezerwacja magistrali.
# Pelne uzasadnienie, log padajacego serwera i rola haka
# RDB_FAULT_PLAN_SWAP_DELAY: run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: launcher.xretractor.29~1, executorsm.executorsm.14~1
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)

# covers: executorsm.executorsm.18~1
# Wymiana planu przy magistrali niedostepnej (Unavailable); bez /dev/shm
# niewykonalna (77).
add_test(
  NAME ${TestID}-bus-unavailable
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash bus_unavailable.sh)

set_tests_properties(${TestID}-bus-unavailable PROPERTIES TIMEOUT 120
                                                          SKIP_RETURN_CODE 77)
