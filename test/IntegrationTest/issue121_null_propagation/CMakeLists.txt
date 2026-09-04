# cd build/Debug && make install && ctest -R issue121_null_propagation -V

# ten test jest wygenerowany przez claude code - sonnet 4.6 prompt: zbuduj test
# integracyjny sprawdzający czy dane typu null przenoszą się wraz z danymi
# źródłowymi. Pobierz dane z pliku tekstowego zawierającego wartości null,
# zbuduj zapytanie które przenosi te dane do strumienia wynikowego. Sprawdź czy
# przeniosły się wartości null.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Verify that null values from the source file propagate correctly through a
# SELECT query into the destination stream.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startują serwer (blokada usługi) -> RUN_SERIAL jawnie (flagi -k/-m są w
# run.sh, więc makro add_test ich nie widzi).
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
