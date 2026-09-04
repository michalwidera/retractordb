# cd build/Debug && make install && ctest -R issue128_string_to_numeric -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Weryfikuje konwersję pól STRING do typów numerycznych za pomocą funkcji
# to_integer(), to_float(), to_double(). Dane wejściowe: STRING "10", "25", "-5"
# → to_integer: 10, 25, -5 to_float:   10, 25, -5 to_double:  10, 25, -5

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startują serwer (blokada usługi) -> RUN_SERIAL jawnie (flagi -k/-m są w
# run.sh, więc makro add_test ich nie widzi).
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
