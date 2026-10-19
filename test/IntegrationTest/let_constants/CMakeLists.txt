# ctest -R let_constants -V

# Stale LET (#440, D4): plan z literalami i ten sam plan zapisany stalymi daja
# identyczny zrzut `xretractor -c` i identyczne wyjscie na tych samych danych.
# Kolejnosc kontroli w verify.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: rqlparser.parser-rql-string.10~1, plansource.parse-plan-text.5~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
