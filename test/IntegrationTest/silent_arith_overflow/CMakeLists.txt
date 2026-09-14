# cd build/Debug && ninja install && ctest -R it_silent_arith_overflow -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Ciche przepelnienie arytmetyki wartosci. INTEGER (int32) i RATIONAL
# (boost::rational<int>) zawijaly sie bez sygnalu: `t[0]*1000000000` przy t =
# 9/1 zapisywalo 410065408/1, a suma dwoch 2e9 w reduktorze i w agregacie okna
# -294967296/1. Przepelnienie jest teraz NULL w polu, tak jak dzielenie przez
# zero (it_null_divide_by_zero).
#
# Test sprawdza wylacznie WARTOSCI, ktore nie zaleza od RDB_OPT_*, wiec nie
# dostaje DISABLED pod ablacja.

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash value.sh)

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 60)
