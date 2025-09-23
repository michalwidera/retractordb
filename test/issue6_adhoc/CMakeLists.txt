# cd build/Debug make install ; ctest -R issue6_adhoc -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
  rm -f temp/str* temp/core* out-ls.txt ; \
  chmod +x ./xlaunch.sh ; \
  ./xlaunch.sh ; \
  ls temp > out-ls.txt ; \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-ls.txt out-ls.txt")
