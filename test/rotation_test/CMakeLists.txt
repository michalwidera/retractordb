get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    rm -f str* ; \
    xretractor query.rql -m 2 ; \
    xretractor query.rql -m 2 ; \
    ls str* | wc -l > count.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol count.pattern count.txt")
