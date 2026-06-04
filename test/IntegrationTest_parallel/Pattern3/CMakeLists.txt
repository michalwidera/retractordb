# Weryfikuje rozwinięcie operatora SELECT * (unfold) na strumieniu z deklaracją
# pliku wyjściowego i retencją. Sprawdza czy kompilator poprawnie rozwija * na
# wszystkie zadeklarowane pola. Wynik porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND sh -c "set -e ; \
    xretractor query-unfold.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
