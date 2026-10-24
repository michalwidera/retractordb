# ctest -R it_xtrdb_lost_lock -V

# cmake-format: off
# Straznik "silnik dziala" w xtrdb bez pliku blokady (#573): plik xretractor_service*.lock
# skasowany albo podmieniony pod zywym silnikiem nie moze otworzyc xtrdb drogi do magazynu.
# Szczegoly w run.sh.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 90)
