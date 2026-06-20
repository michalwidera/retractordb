# Równoległa wersja kompilacyjna issue61_tmpmem: weryfikuje kompilację substratu
# pamięciowego (SUBSTRAT 'memory') w zapytaniu z filtrem. Korzysta z danych w
# IntegrationTest_serial/issue61_tmpmem.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
set(DATA_DIR ${CMAKE_BINARY_DIR}/test/IntegrationTest_serial/issue61_tmpmem)

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND bash -c "set -e && \
    xretractor query-61.rql -c > out.txt && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
