# make install && ctest -R generator_expanded_source -V

# cmake-format: off
# Generator `[$]` nad zrodlem, ktorego szerokosc powstaje dopiero w rozwinieciu:
#   ch[2]  - `ten` ma jedna pozycje listy SELECT (`pair[_]*10`), a dwa pola
#   gap[3] - `win` nad jednopolowym `single` ma przez okno @(1,3) trzy pola
# Do 2026-09-14 kontrola zakresu liczyla szerokosc sprzed rozwiniec i oba
# zapytania odpadaly z "has only 1 element(s)". Wzorzec sprawdzony recznie i
# rowny przebiegowi recznie rozpisanych SELECT-ow; gap[3] siega ostatniego pola.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e && \
  xretractor query.rql -m 8 -f && \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
  bash ../compare.sh --ignore-eol pattern-run.txt out.txt")
