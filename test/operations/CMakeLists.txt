# cd build/Debug make install ; ctest -R operations-run -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xretractor query.rql -k -x & \
    while [ ! -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    xqry -s str3 -k -m 10 > out.txt; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    ${CMAKE_COMMAND} -E compare_files out-pattern.txt out.txt")
set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 300)
