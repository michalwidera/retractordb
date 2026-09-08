# Wymiana planu w trakcie obslugi komendy klienta nie ma prawa zabic serwera.
# Pelne uzasadnienie, stos z rdzenia i rola haka RDB_FAULT_GET_DELAY_MS:
# race.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash race.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)
