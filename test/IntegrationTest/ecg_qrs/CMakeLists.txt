# Potok EKG Pan-Tompkins — flagowy przyklad artykulu (main-debs.tex, sekcja
# "Example D: Pan--Tompkins-Inspired ECG Feature Pipeline") wprowadzony do
# ctest.
#
# Dotad sprawdzala go wylacznie reczna kampania K18 w rdb-experiment. Tymczasem
# scalenie 5f31051 ("Issue 227 precesja") zmienilo stemplowanie okna @ i
# przenioslo rozpietosc okna z ogona do query::logicalOrigin — czyli dotknelo
# dokladnie operatora, na ktorym ten potok stoi.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
