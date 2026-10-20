# cd build/Debug && ninja install && ctest -R it_var_typed -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Jawny typ zmiennej VAR (#546, etap H): licznik UINT, akumulator DOUBLE nad
# polem DOUBLE i prog RATIONAL, wynik wobec wyroczni oracle.py i wobec drugiego
# przebiegu, a `xqry --vars` z typami z adnotacji. Szczegoly w naglowku run.sh.
#
# Test sprawdza wylacznie WARTOSCI i TYPY, ktore nie zaleza od RDB_OPT_*, wiec
# nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 120)
