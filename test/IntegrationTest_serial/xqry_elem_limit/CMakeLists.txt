# make install && ctest -R xqry_elem_limit -V

# Weryfikuje że xqry -m N zwraca dokładnie N wierszy, nawet jeśli źródło
# zawiera więcej rekordów (tutaj: 10 wierszy, limit -m 5).

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    xretractor query.rql -k -x & \
    while [ ! -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    xqry -s dst -k -m 5 > out.txt ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    [ \$(wc -l < out.txt) -eq 5 ]")
