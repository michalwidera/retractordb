get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor queryNoSubstrat.rql -c > out-compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out-compile.txt")

add_test(
  NAME ${TestID}-vg-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    valgrind --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    queryNoSubstrat.rql -c)
