# Weryfikuje że wygenerowany substrat współdzielony między strumieniami
# użytkownika jest poprawnie referencjonowany przez kompilator (issue #96):
# STREAM_TIMEMOVE_core0 musi być reużyty, a nie powielony. Uruchamia też
# valgrind z wykrywaniem wycieków pamięci.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor queryWithSubstrats.rql -c > out-compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out-compile.txt")

add_test(
  NAME ${TestID}-vg-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    valgrind --vgdb=no --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    queryWithSubstrats.rql -c)
