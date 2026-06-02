# Równoległa wersja kompilacyjna issue42_rule: weryfikuje że składnia RULE
# (warunkowe akcje na wartościach strumienia) kompiluje się poprawnie.
# Korzysta z danych w IntegrationTest_serial/issue42_rule.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
set(DATA_DIR ${CMAKE_BINARY_DIR}/test/IntegrationTest_serial/issue42_rule)

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${DATA_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query-42.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
