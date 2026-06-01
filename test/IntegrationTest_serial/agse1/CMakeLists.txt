# make install && ctest -R agse1 -V

# Weryfikuje operator przesunięcia okna czasowego @(start, length) na
# strumieniu. Testy różnych wariantów: @(1,4) przód, @(1,-4) wstecz, @(2,4),
# @(2,2), @(1,1). Wynik odczytany przez xtrdb i porównany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    rm -f *.meta ; \
    xretractor query.rql -m 15 -v > verbose.txt ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ;  \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
