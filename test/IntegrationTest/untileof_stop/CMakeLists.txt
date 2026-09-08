# cd build/Debug && ninja install && ctest -R untileof_stop -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Pozycja 6 z usecases/requested.md: brak trybu "licz do konca wejscia". Test
# porownuje artefakt przebiegu z --until-eof z przebiegiem o recznie dobranym -m
# i sprawdza, ze bez tego trybu zrodlo nadal sie zawija.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
