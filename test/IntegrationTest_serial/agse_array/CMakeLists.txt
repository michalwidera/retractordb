# AGSE must treat one numeric array field exactly like the same flat values
# declared as separate scalar fields.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e && \
    rm -f ./*.desc ./*.meta ./*.shadow fields_window array_window out.txt && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 15 -f > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
