# make install && ctest -R simple_max -V

# Weryfikuje operator .max na strumieniu: str0 zawiera dane z core0+core1,
# str1 = str0.max (wartość maksymalna), str2 = str0 + str1 (złączenie).
# Wynik porównywany z pattern-run.txt via xtrdb.
# Test -vg-2 uruchamia valgrind z wykrywaniem wycieków pamięci.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-vg-2
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    valgrind --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    query.rql -m 5)

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
  xretractor query.rql -m 10 ; \
  while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ;  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out.txt")
