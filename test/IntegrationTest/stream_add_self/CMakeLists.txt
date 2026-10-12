# cd build/Debug && ninja install && ctest -R it_stream_add_self -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Suma strumienia z samym soba (`a+a`, `m+m`). dataModel::fetchForward oddaje
# referencje, a drugi odczyt zrodla niedeklarowanego nadpisuje jego biezacy
# payload - wezel sumy musi wiec wziac lewy skladnik, zanim przeczyta prawy.
# Test sprawdza wylacznie WARTOSCI, ktore nie zaleza od RDB_OPT_*, wiec nie
# dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 60)

add_test(
  NAME ${TestID}-vg-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${IT_VALGRIND} xretractor self.rql -k -f -u)
