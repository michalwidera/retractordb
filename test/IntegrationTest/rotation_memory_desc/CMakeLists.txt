# make install && ctest -R rotation_memory_desc -V

# cmake-format: off
# Plan z :ROTATION startuje na katalogu ze starym .desc strumienia STORAGE MEMORY (sprzed 108a5e94,
# bez RETMEMORY i TYPE). Po przebiegu .desc ma nosic konfiguracje z planu, czyli pierscien z granica.
# Do 2026-09-27 stary .desc zostawal, a memoryFile dostawal pierscien bez granicy (szczegoly w run.sh).
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 60)
