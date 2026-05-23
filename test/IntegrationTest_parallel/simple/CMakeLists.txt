get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
set(DATA_DIR ${CMAKE_BINARY_DIR}/test/IntegrationTest_serial/simple)

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c > out-compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out-compile.txt")

add_test(
  NAME ${TestID}-dot
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c -d -f -t -s > out.dot ; \
    ${CMAKE_COMMAND} -E compare_files pattern-dot.txt out.dot")

add_test(
  NAME ${TestID}-vg-compile
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND
    valgrind --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    query.rql -c)
