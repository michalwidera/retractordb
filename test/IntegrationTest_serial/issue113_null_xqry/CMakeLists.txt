# make install && ctest -R issue113_null_xqry -V

# Weryfikuje że wartości null z pliku źródłowego są poprawnie przesyłane przez
# IPC do xqry i wyświetlane jako "null" w wyjściu (issue #113).
# Dane: wiersz 1 = "10 NULL", wiersz 2 = "20 30". Asercja: grep na obu wzorcach.

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
    xqry -s str_null -k -m 3 > out.txt ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    grep -F '10 null' out.txt ; \
    grep -F '20 30' out.txt")
