# make install && ctest -R lang_showcase -V

# cmake-format: off
# Trzeci przyklad ze strony projektu: w jednym malym planie cechy wlasciwe RQL.
#   core[_]  - dziki indeks, rozwija sie w tyle pol, ile slotow wnosi blok `core@(1,2)`
#   @(1,2)   - okno AGSE po ciagu wartosci (krok 1, szerokosc 2), stad interwal 1/20
#   MAX(win) - reduktor w postaci funkcyjnej, jedno pole z calego rekordu
#   gap[2]   - generator strumieni: `$` = 0 i 1 daje gap$0 i gap$1
# Plan musi zostac na tyle maly, zeby jego DOT dal sie pokazac na stronie.
#
# Test -dot pilnuje tez, ze nazwy z `$` wychodza w DOT w cudzyslowie: bez tego
# `dot` odrzucal plik bledem skladni, a sam wzorzec tekstowy by tego nie wykryl.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c > out-compile.txt && \
    bash ../compare.sh pattern.txt out-compile.txt")

# Render `dot -Tsvg` jest czescia asercji (patrz naglowek): sam wzorzec tekstowy
# nie wykryje pliku DOT, ktorego graphviz nie przyjmie. Graphviz jest jednak
# zaleznoscia opcjonalna wszedzie, nie tylko poza Linuksem: nie ma go macOS ani
# minimalny toolchain CI (`toolchain_required`, gdzie od 2026-08-19 stoi
# `graphviz:optional`, zeby ARM nie ciagnal go z ports.ubuntu.com). Render
# odpada wiec razem z narzedziem, a konfiguracja mowi to wprost - wersja
# bezwarunkowa na Linuksie wywrocila 2026-09-20 joby `build-release` i
# `build-ARM` na `dot: command not found`. Obraz projektu graphviza ma, wiec na
# kazdym commicie render biegnie jak dotad.
set(_dotRender " && dot -Tsvg out.dot -o out.svg")
find_program(RDB_DOT_EXECUTABLE dot)
if(NOT RDB_DOT_EXECUTABLE)
  set(_dotRender "")
  message(
    STATUS
      "graphviz (dot) not found - ${TestID}-dot will check the DOT text only")
endif()

add_test(
  NAME ${TestID}-dot
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c -d -f -t -s > out.dot && \
    bash ../compare.sh pattern-dot.txt out.dot${_dotRender}")

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
  COMMAND ${IT_VALGRIND} xretractor query.rql -c)
