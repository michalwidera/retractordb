# Blad srodowiska (K4) konczy proces kodem EIO z zachowanym planem, a wadliwy
# licznik rotacji jest odmowa planu (K2) - #472. Uzasadnienie czterech
# sprawdzanych sciezek stoi w run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 90)

# Rezerwacja numeru licznika rotacji w katalogu tylko do odczytu; pod rootem
# niewykonalna (77).
add_test(
  NAME ${TestID}-rotation-reserve
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash rotation_reserve.sh)

set_tests_properties(${TestID}-rotation-reserve PROPERTIES TIMEOUT 90
                                                           SKIP_RETURN_CODE 77)
