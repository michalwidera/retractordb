get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xretractor query.rql -m 15 -v > verbose.txt ; \
    xtrdb < term.script > out.txt ;  \
    find ./*.desc | sed 's/\.[^.]*$//' | cut -c 3- | xargs rm -f ; \
    rm -f *.desc ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
