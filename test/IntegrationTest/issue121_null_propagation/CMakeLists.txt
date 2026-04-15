# cd build/Debug && make install && ctest -R issue121_null_propagation -V

# ten test jest wygenerowany przez claude code - sonnet 4.6
# prompt: 
# zbuduj test integracyjny sprawdzający czy dane typu null przenoszą się 
# wraz z danymi źródłowymi. Pobierz dane z pliku tekstowego zawierającego wartości null, 
# zbuduj zapytanie które przenosi te dane do strumienia wynikowego. 
# Sprawdź czy przeniosły się wartości null.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Verify that null values from the source file propagate correctly
# through a SELECT query into the destination stream.
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    xretractor query.rql -k -x & \
    while [ ! -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    xqry -s dst -k -m 4 > out.txt ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -F '10 null' out.txt ; \
    grep -F 'null 20' out.txt ; \
    grep -F '30 40' out.txt")
