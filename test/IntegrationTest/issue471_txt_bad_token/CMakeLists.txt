# cd build/Debug && ninja install && ctest -R it_issue471_txt_bad_token -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Niepoprawny token liczbowy w TEXTFILE daje NULL i jedno ostrzezenie (#471,
# K5). Test sprawdza wylacznie WARTOSCI odczytu zrodla, ktore nie zaleza od
# RDB_OPT_*, wiec nie dostaje DISABLED pod ablacja. xretractor z drzewa buildu,
# bo liczba ostrzezen zalezy od typu buildu (jak signal_stop).

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh $<TARGET_FILE:xretractor> "${CMAKE_BUILD_TYPE}")

set_tests_properties(${TestID}-value PROPERTIES TIMEOUT 60)
