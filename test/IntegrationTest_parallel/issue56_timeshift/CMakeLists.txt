# Równoległa wersja kompilacyjna issue56_timeshift: weryfikuje kompilację
# operatora filtra (>) na połączonych strumieniach (core0+core1 z filtrem >5).
# Korzysta z danych w IntegrationTest_serial/issue56_timeshift.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
set(DATA_DIR ${CMAKE_BINARY_DIR}/test/IntegrationTest_serial/issue56_timeshift)

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND bash -c "set -e && \
    xretractor query-51.rql -c > out.txt && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
