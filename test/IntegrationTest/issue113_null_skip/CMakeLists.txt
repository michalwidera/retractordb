# cd build/Debug && make install && ctest -R issue113_null_skip -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# With -n: all-null rows must be skipped; mixed/non-null rows must appear
add_test(
  NAME ${TestID}-skip-enabled
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xqry --wait-server -s str_null -n -k -m 4 > out_skip.txt & \
    XQRY_PID=$! ; \
    xretractor query.rql -k -x & \
    wait \$XQRY_PID ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -F '10 null' out_skip.txt ; \
    grep -F '20 30' out_skip.txt ; \
    ! grep -qF 'null null' out_skip.txt")

# Without -n: all rows including all-null must appear
add_test(
  NAME ${TestID}-skip-disabled
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xqry --wait-server -s str_null -k -m 4 > out_noskip.txt & \
    XQRY_PID=$! ; \
    xretractor query.rql -k -x & \
    wait \$XQRY_PID ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -F '10 null' out_noskip.txt ; \
    grep -F '20 30' out_noskip.txt ; \
    grep -qF 'null null' out_noskip.txt")
