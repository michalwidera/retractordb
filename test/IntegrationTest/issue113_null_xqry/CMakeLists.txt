# cd build/Debug && make install && ctest -R issue113_null_xqry -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    xqry --wait-server -s str_null -k -m 3 > out.txt & \
    XQRY_PID=$! ; \
    xretractor query.rql -k -x & \
    wait \$XQRY_PID ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -F '10 null' out.txt ; \
    grep -F '20 30' out.txt")
