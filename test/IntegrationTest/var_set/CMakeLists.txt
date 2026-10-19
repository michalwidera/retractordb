# cd build/Debug && ninja install && ctest -R it_var_set -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Akcja reguly DO SET (#531, etap B): histereza, licznik i EMA, szkice uc10,
# uc11, uc12 i uc14, semantyka slotu (D1) i konflikt zapisu w obu kolejnosciach
# regul (D3), kazdy plan sprawdzony wobec wyroczni oracle.py i wobec drugiego
# przebiegu. Dane generuje generate.py; test ich nie generuje. Szczegoly w
# naglowku run.sh.
#
# Test sprawdza wylacznie WARTOSCI i TYPY, ktore nie zaleza od RDB_OPT_*, wiec
# nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 180)
