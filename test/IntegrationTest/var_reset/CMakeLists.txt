# cd build/Debug && ninja install && ctest -R it_var_reset -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Zmienna VAR po ponownym wczytaniu planu wraca do wartosci z deklaracji (#531,
# D6) i zatrzask ustawiony regula DO SET jest tracony do nastepnego
# przekroczenia progu (R2). Scenariusz w run.sh.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
