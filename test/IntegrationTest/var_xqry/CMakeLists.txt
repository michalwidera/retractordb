# cd build/Debug && ninja install && ctest -R it_var_xqry -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Odczyt biezacych wartosci zmiennych VAR przez `xqry --vars` (#531, etap D):
# wartosci z deklaracji przed pierwszym slotem, potem zatwierdzone zapisy SET,
# NULL z przepelnienia, plan bez zmiennych, instancja bezczynna, -y i odmowa
# --jsonl. Scenariusz w run.sh.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
