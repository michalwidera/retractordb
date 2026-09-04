# Blad krytyczny konczy proces czysto (EXIT_FAILURE), a nie SIGSEGV-em ani
# SIGABRT-em. Uzasadnienie obu sprawdzanych sciezek stoi w run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startuje serwer (blokada uslugi) -> RUN_SERIAL jawnie: flagi -k/-m siedza w
# run.sh, wiec makro add_test ich nie widzi.
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
