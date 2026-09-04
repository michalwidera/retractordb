# Weryfikuje kompilację zapytań na strumieniach BYTE z /dev/urandom: bezpośredni
# SELECT, wyrażenie arytmetyczne (+1), łączenie dwóch strumieni z arytmetyką.
# Wynik kompilatora porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND sh -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
