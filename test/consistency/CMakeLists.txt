get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    sh -c
    "set -e ; \
  xretractor -c query-consitency.rql > out_compile.txt ; \
  xretractor query-consitency.rql -m 5 -v -l > verbose.txt ; \
  xtrdb < term.script > out_xtrdb.txt ; \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern_compile.txt out_compile.txt ; \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern_xtrdb.txt out_xtrdb.txt"
)
