# Weryfikuje operacje xtrdb rread i getpos na strumieniu TEXTSOURCE zawierającym
# wartość NULL (issue #113): data.txt ma wiersz "10 NULL" — sprawdza czy
# odczyt wsteczny i pozycjonowanie działają poprawnie przy null w źródle tekstowym.
# Wynik porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ;  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
