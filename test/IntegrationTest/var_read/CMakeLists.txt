# ctest -R var_read -V

# Zmienne VAR (#531, etap A): plan, w ktorym zmienne nie maja zadnego SET, daje
# identyczne wyjscie jak ten sam plan ze stalymi LET o tych samych wartosciach.
# Kolejnosc kontroli w verify.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: rqlparser.parser-rql-string.15~1, datamodel.data-model.16~1, streaminstance.stream-instance.10~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
