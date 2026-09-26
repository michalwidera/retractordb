# cd build/Debug && ninja install && ctest -R it_expr_corpus -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Korpus wyrazen arytmetycznych (#289): plany wlasne (po jednym na typ, typy
# mieszane, napisy, gleboki program) i wyrazenia z list SELECT testow
# systemowych przeniesione na wspolne zrodlo. Pliki generuje generate.py; test
# ich nie generuje. Kazdy plan sprawdzany wobec wzorca i wobec wyroczni
# niezaleznej oracle.py, ktora liczy wartosci i typy z modelu jezyka, nie z kodu
# silnika.
#
# Test sprawdza wylacznie WARTOSCI i TYPY, ktore nie zaleza od RDB_OPT_*, wiec
# nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 120)
