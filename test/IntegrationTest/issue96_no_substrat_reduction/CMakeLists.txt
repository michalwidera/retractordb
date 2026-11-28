# Weryfikuje że strumienie zdefiniowane przez użytkownika o identycznej
# strukturze NIE są scalane przez kompilator (issue #96): str1 i str2 to
# niezależne strumienie użytkownika (nie automatyczne substraty), więc oba muszą
# pozostać oddzielne.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor queryNoSubstrat.rql -c > out-compile.txt && \
    bash ../compare.sh pattern.txt out-compile.txt")
