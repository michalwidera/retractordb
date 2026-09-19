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

# Graphviz jest zalezoscia OPCJONALNA (patrz issue31_doc): bez programu `dot`
# renderowanie grafu planu nie ma czym sie odbyc, a "dot: command not found" nie
# jest regresja silnika. Rejestrujemy wtedy pozostale testy tego katalogu i
# pomijamy sam render - tak samo jak robi to issue31_doc.
find_program(RDB_DOT_EXECUTABLE dot)

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c > out-compile.txt && \
    bash ../compare.sh pattern.txt out-compile.txt")

# Renderowanie grafu wymaga graphviz, a to zaleznosc OPCJONALNA (ten sam wzorzec
# co w issue31_doc). Bez programu `dot` odpada SAM RENDER - porownanie
# wygenerowanego tekstu DOT z plikiem wzorcowym, czyli to, co ten test naprawde
# sprawdza, zostaje. Z zainstalowanym graphvizem polecenie jest co do znaku
# takie jak dotad.
if(RDB_DOT_EXECUTABLE)
  set(_dotRender
      " && dot -Tsvg out.dot -o out.svg 2> dot-stderr.txt && test ! -s dot-stderr.txt"
  )
else()
  set(_dotRender "")
  message(
    STATUS
      "graphviz (dot) not found - ${TestID}-dot will check the DOT text only")
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
