# cd build/Debug && ninja install && ctest -R it_rule_string_compare -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Porownanie napisow w warunku RULE konczylo proces przez FatalError w
# boolCast(), a wynik porownania (napis "1"/"0") logika czytala jako prawde.
# Test liczy odpalenia kazdej reguly. Sprawdza wylacznie wartosci warunkow,
# ktore nie zaleza od RDB_OPT_*.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
