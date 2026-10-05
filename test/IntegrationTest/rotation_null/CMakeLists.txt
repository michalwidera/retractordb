get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Dwie sesje o roznych wzorcach NULL: dane i indeks musza nosic ten sam numer.
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)
set_tests_properties(${TestID} PROPERTIES TIMEOUT 60)
