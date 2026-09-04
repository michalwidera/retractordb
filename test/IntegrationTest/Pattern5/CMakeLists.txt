# Weryfikuje operacje xtrdb na rekordzie wielotypowym (STRING, INTEGER, BYTE,
# FLOAT): tworzenie bazy, append, read, printt, list/rlist (przód/tył), input,
# write. Wynik porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    sh -c "set -e ; \
    rm -f *.meta ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
