get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-waterfall
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "
xretractor query-lnx.rql -m 10
rm -f core*
rm -f str*
")

add_test(
  NAME ${TestID}-workflow
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ./test-workflow.sh query-lnx.rql str2 str3)

add_test(
  NAME ${TestID}-all-operators
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ./test-workflow.sh query-all.rql str2 str2)

set_tests_properties(
  ${TestID}-waterfall ${TestID}-workflow ${TestID}-all-operators
  PROPERTIES RUN_SERIAL TRUE TIMEOUT 30)
