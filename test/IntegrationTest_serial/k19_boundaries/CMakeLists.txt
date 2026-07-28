# K19/G16: operator-tail audit at the observable boundary. The golden output
# distinguishes legitimate NULL values carried by a complete AGSE window from
# synthetic NULLs caused by reading beyond retained or not-yet-produced data.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e && \
    rm -f *.meta source agse agse_mirror sub_same sub2 sum3 && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 12 > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
