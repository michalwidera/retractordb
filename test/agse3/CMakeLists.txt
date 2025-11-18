get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xretractor query.rql -m 15 -v -l> verbose.txt ; \
    xtrdb < term.script > out.txt ;  \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
