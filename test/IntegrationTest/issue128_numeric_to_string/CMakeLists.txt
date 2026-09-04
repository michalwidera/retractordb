# cd build/Debug && make install && ctest -R issue128_numeric_to_string -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Weryfikuje konwersję typów numerycznych (INTEGER, FLOAT) do STRING za pomocą
# funkcji to_string(). Dane wejściowe: INTEGER 42 i FLOAT 3.14 → STRING "42" i
# "3.140000".

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startują serwer (blokada usługi) -> RUN_SERIAL jawnie (flagi -k/-m są w
# run.sh, więc makro add_test ich nie widzi).
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
