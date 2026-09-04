# cd build/Debug && ninja install && ctest -R noclock_offline -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Pozycja 7 z usecases/requested.md: brak trybu bez taktowania zegarem sciennym.
# Test porownuje artefakt przebiegu taktowanego i przebiegu --no-clock bajtowo
# oraz sprawdza, ze drugi jest realnie krotszy.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
