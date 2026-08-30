# A numeric array field T[N] and N scalar fields T describe the same record, so
# every DERIVED stream must produce the same values from both. Covers the
# projection, the time shift and the reducer; the window itself is covered by
# agse_array.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e && \
    rm -f ./*.desc ./*.meta ./*.shadow fields_pass array_pass fields_shift array_shift fields_sum array_sum fields_woven array_woven mixed_woven out.txt && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 15 -f > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
