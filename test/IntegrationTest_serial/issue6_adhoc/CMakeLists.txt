# make install && ctest -R issue6_adhoc -V

# Weryfikuje tryb zapytania ad-hoc (issue #6): xqry -a 'SELECT ...' definiuje i
# wykonuje zapytanie w locie bez wcześniej skompilowanego pliku .rql. xlaunch.sh
# startuje xretractor, wysyła zapytanie ad-hoc przez xqry, następnie zatrzymuje
# serwer. Asercja: pliki strumieni w temp/ zgodne z pattern-ls.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e && \
  chmod +x ./xlaunch.sh && \
  ./xlaunch.sh && \
  ls temp | grep --invert-match '\\.shadow$' > out-ls.txt && \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-ls.txt out-ls.txt")

# Startuje serwer przez xlaunch.sh (blokada usługi) -> RUN_SERIAL.
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
