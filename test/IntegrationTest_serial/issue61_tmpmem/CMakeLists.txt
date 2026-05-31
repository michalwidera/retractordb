# make install && ctest -R issue61_tmpmem -V

# Weryfikuje działanie substratu pamięciowego (SUBSTRAT 'memory', issue #61).
# Dane z core0+core1 przetwarzane z filtrem > 5 i zapisywane w pamięci zamiast
# na dysku. Wynik odczytany przez xtrdb i porównany z pattern-run.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
  xretractor query-61.rql -m 10 ; \
  while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out-run.txt ;  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out-run.txt")
