# make install && ctest -R lang_showcase -V

# Trzeci przyklad ze strony projektu: w jednym malym planie cechy wlasciwe RQL.
#   core[_]  - dziki indeks, rozwija sie w tyle pol, ile slotow wnosi blok `core@(1,2)`
#   @(1,2)   - okno AGSE po ciagu wartosci (krok 1, szerokosc 2), stad interwal 1/20
#   MAX(win) - reduktor w postaci funkcyjnej, jedno pole z calego rekordu
#   gap[2]   - generator strumieni: `$` = 0 i 1 daje gap$0 i gap$1
# Plan musi zostac na tyle maly, zeby jego DOT dal sie pokazac na stronie.
#
# Test -dot pilnuje tez, ze nazwy z `$` wychodza w DOT w cudzyslowie: bez tego
# `dot` odrzucal plik bledem skladni, a sam wzorzec tekstowy by tego nie wykryl.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c > out-compile.txt && \
    bash ../compare.sh pattern.txt out-compile.txt")

add_test(
  NAME ${TestID}-dot
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c -d -f -t -s > out.dot && \
    bash ../compare.sh pattern-dot.txt out.dot && \
    dot -Tsvg out.dot -o out.svg")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e && \
  xretractor query.rql -m 12 -f && \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt &&  \
  bash ../compare.sh --ignore-eol pattern-run.txt out.txt")

add_test(
  NAME ${TestID}-vg-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    valgrind --vgdb=no --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    query.rql -c)
