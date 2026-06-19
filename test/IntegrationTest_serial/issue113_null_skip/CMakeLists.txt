# cd build/Debug && make install && ctest -R issue113_null_skip -V

# Weryfikuje flagę -n (skip-null) w xqry (issue #113): z -n wiersze w całości
# null są pomijane (brak "null null" w wyjściu), mieszane i niepuste wiersze
# muszą się pojawić. Bez -n wszystkie wiersze łącznie z all-null muszą być
# widoczne.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# With -n: all-null rows must be skipped; mixed/non-null rows must appear

add_test(
  NAME ${TestID}-skip-enabled
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh skip)

add_test(
  NAME ${TestID}-skip-disabled
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh noskip)

# Startują serwer (blokada usługi) -> RUN_SERIAL jawnie (flagi -k/-m są w
# run.sh, więc makro add_test ich nie widzi).
set_tests_properties(${TestID}-skip-enabled ${TestID}-skip-disabled
                     PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
