# cd build/Debug && make install && ctest -R issue128_numeric_to_string -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Weryfikuje konwersję typów numerycznych (INTEGER, FLOAT) do STRING za pomocą
# funkcji to_string(). Dane wejściowe: INTEGER 42 i FLOAT 3.14 → STRING "42" i
# "3.140000".
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    xretractor query.rql -k -x & \
    while [ ! -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    xqry -s dst -k -m 3 > out.txt ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -F '42_test' out.txt ; \
    grep -F '3.140000' out.txt ; \
    grep -F '100_test' out.txt ; \
    grep -F -- '-1.500000' out.txt ; \
    grep -F '7_test' out.txt ; \
    grep -F '0.000000' out.txt ; \
    grep -F 'STRING dst_0[21]' temp/dst.desc ; \
    grep -F 'STRING dst_1[16]' temp/dst.desc")
