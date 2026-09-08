get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-waterfall
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "
xretractor query-lnx.rql -m 10
rm -f core*
rm -f str*
")

add_test(
  NAME ${TestID}-workflow
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ./test-workflow.sh query-lnx.rql str2 str3)

add_test(
  NAME ${TestID}-all-operators
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ./test-workflow.sh query-all.rql str2 str2)

set_tests_properties(${TestID}-waterfall ${TestID}-workflow
                     ${TestID}-all-operators PROPERTIES TIMEOUT 30)

# Testy kompilacyjne tego katalogu, do 2026-09-04 osobny katalog
# IntegrationTest_parallel/Data. Podzial na drzewo "serial" i "parallel" stracil
# sens, gdy testy serii dostaly przestrzenie nazw i zaczely biec rownoczesnie;
# te testy i tak pracowaly na danych stad, tylko z drugiego CMakeLists.

add_test(
  NAME ${TestID}-on-query
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND xretractor -q query-lnx.rql -c)

add_test(
  NAME ${TestID}-banned-artifacts
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND fail-if-file-exist.sh xrdb)

add_test(
  NAME ${TestID}-presenter
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "xretractor query-lnx.rql -c")

set_tests_properties(${TestID}-presenter PROPERTIES DEPENDS ${TestID}-on-query)
