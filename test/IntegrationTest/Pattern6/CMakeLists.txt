# Weryfikuje kompilację operatora okna czasowego @(n,m) w wariantach do przodu
# @(1,10) i wstecz @(1,-10) na strumieniu BYTE. Dodatkowo uruchamia valgrind z
# wykrywaniem wycieków pamięci podczas kompilacji.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")

add_test(
  NAME ${TestID}-vg-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    valgrind --vgdb=no --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    query.rql -c)
