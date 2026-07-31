# make install && ctest -R issue217_client_diag_stderr -V

# Weryfikuje że xqry zgłasza awarię na STDERR, a nie wyłącznie na STDOUT.
#
# Scenariusz: - xqry wołany bez działającego serwera - asercja: kod wyjścia
# różny od zera ORAZ niepusty stderr

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Dotyka obiektów IPC serwera -> nie może biec równolegle z testami startującymi
# xretractor (flagi -k/-m są w run.sh, więc makro add_test ich nie widzi).
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
