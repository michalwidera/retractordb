# make install && ctest -R xqry_elem_limit -V

# Weryfikuje że parametr -m N (elimitqry) w xqry ogranicza liczbę odebranych
# elementów do dokładnie N, nawet gdy źródło zawiera więcej rekordów.
#
# Scenariusz:
#   - xretractor wczytuje data.txt (10 wierszy całkowitych) jako strumień ONESHOT
#   - xqry pobiera strumień dst z limitem -m 5 i zapisuje wynik do out.txt
#   - po zakończeniu xqry wysyła sygnał kill do xretractor (-k)
#   - asercja: wc -l out.txt == 5

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
