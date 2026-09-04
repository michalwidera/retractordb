# make install && ctest -R wide_from_names -V

# Sufit NAME_MAX na nazwie substratu. Plan stoi okrakiem na progu z
# compiler::composeStreamName(): wezel 12-skladnikowy zostaje czytelny (192
# bajty), 13-skladnikowy dostaje skrot. Sprawdza obie galezie, wytworzenie
# artefaktu na dysku i poprawnosc redukcji nad nim.
#
# Test nie zalezy od zadnego przelacznika RDB_OPT_*: nazwa wezla powstaje w
# extractIntermediateStreams(), przed calym lancuchem optymalizacji.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
