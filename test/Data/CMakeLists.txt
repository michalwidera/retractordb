get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}-on-query
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND xretractor -q query-lnx.rql -c)

# Purpose of this test is to fail if someone will create xrdb artifact If such
# file will appear in .local/bin folder or in a path this will cause to system
# hang during startup My fail count #2

add_test(
  NAME ${TestID}-banned-artifacts
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND fail-if-file-exist.sh xrdb)

add_test(
  NAME ${TestID}-dumper
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND sh -c "xretractor query-lnx.rql -c")

set_tests_properties(${TestID}-dumper PROPERTIES DEPENDS ${TestID}-on-query)

add_test(
  NAME ${TestID}-waterfall
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND sh -c "rm -f str*; rm -f core*; xretractor query-lnx.rql -m 10")

add_test(
  NAME ${TestID}-workflow
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND test-workflow.sh query-lnx.rql)

add_test(
  NAME ${TestID}-all-operators
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND test-all-operators.sh query-all.rql)

set_tests_properties(${TestID}-waterfall ${TestID}-workflow
                     ${TestID}-all-operators PROPERTIES RUN_SERIAL TRUE)
