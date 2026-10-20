# cd build/Debug && ninja install && ctest -R it_let_typed -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Jawny typ stalej LET (#547): stala DOUBLE nad polem DOUBLE obok stalej bez
# adnotacji (FLOAT), stala RATIONAL w wyrazeniu i stala UINT w porownaniu z
# polem UINT, wynik wobec wyroczni oracle.py. Szczegoly w naglowku run.sh.
#
# Test sprawdza wylacznie WARTOSCI i TYPY, ktore nie zaleza od RDB_OPT_*, wiec
# nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 120)
