# cd build/Debug && ninja install && ctest -R fncall_runtime_case -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Gramatyka zna funkcje wyłącznie w postaci 'Sqrt', 'Ceil', 'Floor', a ewaluator
# dopasowywał nazwy tylko małymi literami — plan kompilował się i wywracał
# dopiero w wykonaniu. Test uruchamia plan i sprawdza wartości.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
