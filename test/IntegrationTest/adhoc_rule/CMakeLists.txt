# make install && ctest -R adhoc_rule -V

# Regula dolaczana przez kanal ad-hoc (issue_238): uzbrojenie na zywym planie,
# granica historii (ujemny zakres DUMP siega tylko rekordow po dolaczeniu) oraz
# bateria odmow, z ktorych kazda ma zostawic serwer przy zyciu. Scenariusz i
# uzasadnienie obserwabli sa w adhoc_rule.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash adhoc_rule.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
