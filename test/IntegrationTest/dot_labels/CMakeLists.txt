# make install && ctest -R dot_labels -V

# cmake-format: off
# Etykiety DOT dla cech jezyka, ktore trafiaja do wezlow shape=record.
#   lit, quote - literaly ze znakami skladni rekordu (| { } < >) oraz " i \
#   fn, lit    - CALL(Sqrt) i CALL2(to_string[8]) z nazwa funkcji
#   w          - dwie grupy okna (jedna wspoldzielona przez MIN i MAX), origin=2
#   r1         - DO SYSTEM z | i >, widoczne tylko z -u
#   ch[2]      - nazwy z `$`
#
# Wzorzec tekstowy nie wykryje etykiety, ktorej dot nie przyjmuje, stad dot -Tsvg
# z pustym stderr: ostrzezenie tez jest bledem.
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
# nie wykryje pliku DOT, ktorego graphviz nie przyjmie. Na Linuksie zostaje wiec
# BEZWARUNKOWY - brak programu `dot` ma dac czerwony test, a nie po cichu
# slabsze sprawdzenie. Poza Linuksem graphviz jest zaleznoscia opcjonalna (macOS
# go nie ma): bez `dot` odpada sam render, a konfiguracja mowi to wprost.
set(_dotRender
    " && dot -Tsvg out.dot -o out.svg 2> dot-stderr.txt && test ! -s dot-stderr.txt"
)
if(NOT RDB_OS_LINUX)
  find_program(RDB_DOT_EXECUTABLE dot)
  if(NOT RDB_DOT_EXECUTABLE)
    set(_dotRender "")
    message(
      STATUS
        "graphviz (dot) not found - ${TestID}-dot will check the DOT text only")
  endif()
endif()

add_test(
  NAME ${TestID}-dot
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c -d -f -t -s -u > out.dot && \
    bash ../compare.sh pattern-dot.txt out.dot${_dotRender}")

add_test(
  NAME ${TestID}-vg-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${IT_VALGRIND} xretractor query.rql -c)
