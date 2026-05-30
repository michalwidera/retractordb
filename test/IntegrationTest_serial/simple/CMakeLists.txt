# make install && ctest -R simple -V

# Podstawowy test dymny arytmetyki na połączonych strumieniach.
# core0 (rate 0.1) i core1 (rate 0.2) łączone operatorem +; SELECT oblicza
# wyrażenia arytmetyczne na polach str1. Wynik porównywany z pattern-run.txt.
# Test -vg-run dodatkowo uruchamia valgrind z wykrywaniem wycieków pamięci.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-vg-run
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
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ;  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out.txt")
