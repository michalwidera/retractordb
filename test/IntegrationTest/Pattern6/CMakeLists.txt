# Weryfikuje kompilację operatora okna czasowego @(n,m) w wariantach do przodu
# @(1,10) i wstecz @(1,-10) na strumieniu BYTE.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
