# cd build/Debug make install ; ctest -R issue42_rule -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query-42.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
  rm -f str1 str1.desc ; \
  xretractor query-42.rql -m 10 ; \
  xtrdb < term.script > out-run.txt ;  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out-run.txt")
