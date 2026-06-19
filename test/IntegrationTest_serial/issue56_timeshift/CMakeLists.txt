# make install && ctest -R issue56_timeshift -V

# Weryfikuje operator filtra (> 5) na połączonych strumieniach (issue #57).
# core0 + core1 z filtrem > 5 — tylko rekordy spełniające warunek trafiają do
# str1. Wynik porównywany z pattern-run.txt via xtrdb.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "
set -e
xretractor query-51.rql -m 10
while [ -f /tmp/xretractor_service.lock ]
do sleep 0.1
done
${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out-run.txt
${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out-run.txt
")
