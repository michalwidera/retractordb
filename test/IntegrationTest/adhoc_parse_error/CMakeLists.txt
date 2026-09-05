# make install && ctest -R adhoc_parse_error -V

# Straznik odpornosci serwera na bledne zapytanie ad-hoc: `xqry -a "ml"` ma
# zostac odrzucone, a instancja xretractora ma to przezyc i obsluzyc kolejne
# zapytanie. Uzasadnienie i kolejnosc kontroli sa w adhoc.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash adhoc.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
