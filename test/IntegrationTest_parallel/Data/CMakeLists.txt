get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
set(DATA_DIR ${CMAKE_BINARY_DIR}/test/IntegrationTest_serial/Data)

add_test(
  NAME ${TestID}-on-query
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND xretractor -q query-lnx.rql -c)

add_test(
  NAME ${TestID}-banned-artifacts
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND fail-if-file-exist.sh xrdb)

add_test(
  NAME ${TestID}-presenter
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND bash -c "xretractor query-lnx.rql -c")

set_tests_properties(${TestID}-presenter PROPERTIES DEPENDS ${TestID}-on-query)
