# Weryfikuje operacje xtrdb na rekordzie wielotypowym (STRING, INTEGER, BYTE,
# FLOAT): tworzenie bazy, append, read, printt, list/rlist (przód/tył), input,
# write. Wynik porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    sh -c "set -e && \
    rm -f *.meta && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")

# Wariant -vg-: ten sam scenariusz z programami pod valgrindem (it_add_vg_test).
# Komendy payload, storage i drop xtrdb na rekordzie wielotypowym.
it_add_vg_test(
  NAME
  ${TestID}-vg-run
  WORKING_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
  sh
  -c
  "set -e && \
    rm -f *.meta && \
    ${IT_VG_SHIM}/xtrdb noprompt < term.script > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
set_tests_properties(${TestID}-vg-run PROPERTIES TIMEOUT 300)
