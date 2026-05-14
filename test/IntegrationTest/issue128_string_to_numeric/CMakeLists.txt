# cd build/Debug && make install && ctest -R issue128_string_to_numeric -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Weryfikuje konwersję pól STRING do typów numerycznych
# za pomocą funkcji to_integer(), to_float(), to_double().
# Dane wejściowe: STRING "10", "25", "-5" →
#   to_integer: 10, 25, -5
#   to_float:   10, 25, -5
#   to_double:  10, 25, -5
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    valgrind --error-exitcode=1 --leak-check=full --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xqry --wait-server -s dst -k -m 3 > out.txt & \
    XQRY_PID=$! ; \
    xretractor query.rql -k -x & \
    wait \$XQRY_PID ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -F '11 10 10' out.txt ; \
    grep -F '26 25 25' out.txt ; \
    grep -F -- '-4 -5 -5' out.txt")
