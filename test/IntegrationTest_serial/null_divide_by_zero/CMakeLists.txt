# cd build/Debug && make install && ctest -R null_divide_by_zero -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Dzielenie przez zero jest wartoscia pochlaniajaca, nie bledem: 100/0 daje
# NULL, a strumien produkuje kolejne rekordy. Dane 4, 0, 5 -> 25, null, 20.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startuja serwer (blokada uslugi) -> RUN_SERIAL jawnie (flagi -k/-m sa w
# run.sh, wiec makro add_test ich nie widzi).
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
