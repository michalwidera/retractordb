# cd build/Debug make install ; ctest -R issue61_tmpmem-run -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query-61.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
  rm -f str1 str1.desc ; \
  rm -f STREAM_ADD_core0_core1 STREAM_ADD_core0_core1.desc ; \
  xretractor query-61.rql -m 10 ; \
  xtrdb < term.script > out-run.txt ;  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out-run.txt")
