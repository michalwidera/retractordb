# cd build/Debug && make install && ctest -R issue113_meta_xtrdb -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Step 1: run xretractor to produce str_null stream + meta index

# Step 2: open str_null in xtrdb and invoke "meta", capture output

# Step 3: verify key properties of the meta output

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    valgrind --error-exitcode=1 --leak-check=full --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xqry --wait-server -s str_null -k -m 3 > /dev/null & \
    XQRY_PID=$! ; \
    xretractor query.rql -k -x & \
    wait \$XQRY_PID ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    test -f temp/str_null.meta ; \
    test $(stat -c%s temp/str_null.meta) -gt 16 ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ; \
    grep -q 'meta: temp/str_null.meta' out.txt ; \
    ! grep -q 'meta file not found' out.txt")
