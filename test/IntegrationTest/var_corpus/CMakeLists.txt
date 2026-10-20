get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Korpus uc10-uc16 z #531/#538: wartosci, NULL, kolejnosc, zrzuty i determinizm.
# Lokalna rejestracja wystarcza: rodzic odkrywa podkatalogi automatycznie. Zadna
# asercja nie zalezy od ksztaltu zoptymalizowanego planu.
add_test(
  NAME ${TestID}-oracle
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 test_oracle.py)

foreach(
  case
  uc10
  uc11
  uc12
  uc13
  uc14
  uc15
  uc16
  uc10-restart)
  add_test(
    NAME ${TestID}-${case}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND bash run.sh ${case} $<TARGET_FILE:xretractor>)
  set_tests_properties(${TestID}-${case} PROPERTIES TIMEOUT 120)

  add_test(
    NAME ${TestID}-vg-${case}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND bash run.sh ${case} $<TARGET_FILE:xretractor> ${IT_VALGRIND})
  set_tests_properties(${TestID}-vg-${case} PROPERTIES TIMEOUT 300)
endforeach()
