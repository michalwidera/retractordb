# cd build/Debug && make install && ctest -R issue113_meta_internal -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e ; \
    mkdir -p temp ; \
    rm -f temp/str_null temp/str_null.* temp/core0 temp/core0.* ; \
    for i in $(seq 1 30); do [ ! -f /tmp/xretractor_service.lock ] && break; sleep 1; done ; \
    [ ! -f /tmp/xretractor_service.lock ] ; \
    xretractor query.rql -k -x & \
    for i in $(seq 1 30); do [ -f /tmp/xretractor_service.lock ] && break; sleep 1; done ; \
    [ -f /tmp/xretractor_service.lock ] ; \
    xqry -s str_null -k -m 5 > /dev/null ; \
    for i in $(seq 1 30); do [ ! -f /tmp/xretractor_service.lock ] && break; sleep 1; done ; \
    [ ! -f /tmp/xretractor_service.lock ] ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ; \
    grep -Fq 'header size: 16' out.txt ; \
    grep -Fq 'entry size: 17' out.txt ; \
    grep -Fq 'sampling interval: 1/1' out.txt ; \
    grep -Fq 'entry[0] count:1 gap:0 bitsetSize:2 bitsetHex:03' out.txt ; \
    grep -Fq 'entry[1] count:2 gap:0 bitsetSize:2 bitsetHex:02' out.txt ; \
    grep -Fq 'entry[2] count:1 gap:0 bitsetSize:2 bitsetHex:00' out.txt ; \
    grep -Fq 'entries: 3' out.txt")
