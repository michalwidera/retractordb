# cd build/Debug && ninja install && ctest -R it_unary_ops -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Wartosci `-a`, `+a` i `~a` nad polami od parsera do artefaktu (#328): `-a` ==
# `~a` == bitowe NOT dla BYTE i UINT, przepelnienie INTEGER jako NULL, wiazanie
# `-a+1` jako `(-a)+1` i minus w warunku RULE. Test sprawdza wylacznie WARTOSCI
# i TYPY, ktore nie zaleza od RDB_OPT_*, wiec nie dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 60)
