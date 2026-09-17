# cd build/Debug && ninja install && ctest -R it_reducer_float_max_avg_count -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Reduktory nad polami rekordu (streamInstance::reduceFieldsToPayload).
#
# MAX nad FLOAT/DOUBLE startowal od numeric_limits<T>::min() - najmniejszej
# DODATNIEJ liczby - wiec maksimum samych ujemnych wartosci bylo 1.17549e-38.
# AVG dzielil przez liczbe pol rzutowana na uint8_t: 256 pol konczylo silnik
# wyjatkiem bad_rational, 257 dzielilo przez 1.
#
# Test sprawdza wylacznie WARTOSCI, ktore nie zaleza od RDB_OPT_*, wiec nie
# dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash value.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 60)
