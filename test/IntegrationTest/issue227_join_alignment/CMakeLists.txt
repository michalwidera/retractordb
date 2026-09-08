# make install && ctest -R issue227_join_alignment -V
#
# Regresja precesji (issue #227): strumien pochodny musi byc zlaczany z ta
# chwila zrodla, ktorej odpowiada. Test pokrywa luke, ktora zostawily testy
# sasiednie — dsp i issue96_substrat_reference porownuja wylacznie listing
# kompilacji, wiec zmiana TRESCI zlaczenia przechodzila przez nie niezauwazona.
#
# Oczekiwania sa w verify.sh wyprowadzane z definicji operatorow, nie przepisane
# z wyjscia silnika.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

add_test(
  NAME ${TestID}-adhoc-origin
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify_adhoc.sh)

set_tests_properties(${TestID}-run ${TestID}-adhoc-origin PROPERTIES TIMEOUT 60)
