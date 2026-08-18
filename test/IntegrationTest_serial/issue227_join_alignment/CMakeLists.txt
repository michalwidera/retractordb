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

# Test uruchamia silnik, wiec nie moze biec rownolegle z innymi takimi testami:
# dziela ${TMPDIR:-/tmp}/xretractor_service.lock i pamiec wspoldzielona boost.
# Automatyczne wykrywanie w IntegrationTest_serial/CMakeLists.txt patrzy na
# argumenty add_test i przy `bash verify.sh` nie widzi ani `-m N`, ani `xqry` —
# trzeba oznaczyc recznie (tak samo robi r1_identity_nulls).
set_tests_properties(${TestID}-run ${TestID}-adhoc-origin
                     PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
