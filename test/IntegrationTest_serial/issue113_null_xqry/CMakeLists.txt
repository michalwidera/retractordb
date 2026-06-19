# make install && ctest -R issue113_null_xqry -V

# Weryfikuje że wartości null z pliku źródłowego są poprawnie przesyłane przez
# IPC do xqry i wyświetlane jako "null" w wyjściu (issue #113). Dane: wiersz 1 =
# "10 NULL", wiersz 2 = "20 30". Asercja: grep na obu wzorcach.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startują serwer (blokada usługi) -> RUN_SERIAL jawnie (flagi -k/-m są w
# run.sh, więc makro add_test ich nie widzi).
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
