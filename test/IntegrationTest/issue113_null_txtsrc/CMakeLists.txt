# Weryfikuje operacje xtrdb rread i getpos na strumieniu TEXTSOURCE zawierającym
# wartość NULL (issue #113): data.txt ma wiersz "10 NULL" - sprawdza czy odczyt
# wsteczny i pozycjonowanie działają poprawnie przy null w źródle tekstowym.
# Wynik porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt &&  \
  bash ../compare.sh --ignore-eol pattern.txt out.txt")

# Wariant -vg-: ten sam scenariusz z programami pod valgrindem (it_add_vg_test).
# Jedyny scenariusz dla cmdFieldAccess.cpp (rread, getpos) na TEXTSOURCE z NULL.
it_add_vg_test(
  NAME
  ${TestID}-vg-run
  WORKING_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
  bash
  -c
  "set -e && \
  ${IT_VG_SHIM}/xtrdb noprompt < term.script > out.txt &&  \
  bash ../compare.sh --ignore-eol pattern.txt out.txt")
set_tests_properties(${TestID}-vg-run PROPERTIES TIMEOUT 300)
