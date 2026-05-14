# cd build/Debug make install ; ctest -R operations-run -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    rm -f *.meta *.desc str1 str2 str3 core0 core1 ; \
    valgrind --error-exitcode=1 --leak-check=full --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xqry --wait-server -s str3 -k -m 9 > out.txt & \
    XQRY_PID=$! ; \
    xretractor query.rql -k -x & \
    wait \$XQRY_PID ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -q '100' out.txt ; \
    grep -q '110' out.txt ; \
    grep -q '22' out.txt")
# set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 900)
