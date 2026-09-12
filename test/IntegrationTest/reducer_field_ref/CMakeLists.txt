# cd build/Debug && ninja install && ctest -R it_reducer_field_ref -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Odwolanie do pola reduktora w liscie SELECT (znalezisko przy zamykaniu pozycji
# 16 w paper-arXiv/usecases/requested.md).
#
# `SELECT avg STREAM o FROM AVG(src)` kompilowalo sie i ginelo w wykonaniu na
# `Unsupported token in expressionEvaluator`, przy zerze rekordow. Bramka `Check
# result:` zamyka ten wzorzec w kompilacji i nazywa obejscie.
#
# DWA wpisy, a nie jeden: asercja o ksztalcie planu (czy sie kompiluje) i
# asercja o policzonej wartosci nie moga dzielic jednego wpisu ctest — patrz
# regula przy issue202_hash_shift_e2e w CLAUDE.md.

add_test(
  NAME ${TestID}-shape
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash shape.sh)

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash value.sh)

set_tests_properties(${TestID}-shape ${TestID}-value PROPERTIES TIMEOUT 60)
