get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/check_loop.sh)
