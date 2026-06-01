# make install && ctest -R rotation_test -V

# Weryfikuje mechanizm rotacji plików binarnych strumieni (ROTATION). Uruchamia
# xretractor dwukrotnie z -m 2; po każdym przebiegu sprawdza czy liczba plików
# str* (bez .shadow) jest zgodna z oczekiwaną (count.pattern). Pętla oczekiwania
# na lock file między uruchomieniami chroni przed race condition.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    rm -f str* ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    xretractor query.rql -m 2 ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    xretractor query.rql -m 2 ; \
    ls str* | grep --invert-match '\\.shadow$' | wc -l > count.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol count.pattern count.txt")

# note : the while loop is to wait for the service to release the lock file one
# test on CI failed because the service was still running from the first command
# when the second command was issued. see:
# https://unix.stackexchange.com/questions/185283/how-do-i-wait-for-a-file-in-the-shell-script
set_tests_properties(${TestID} PROPERTIES TIMEOUT 60)
