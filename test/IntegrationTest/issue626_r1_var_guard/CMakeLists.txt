# covers: compiler.compiler.10~1, compiler.compiler.17~1
#
# Regresja #626: R1 nie moze zmieniac wartosci rekordow czytajacych VAR. `-value`
# porownuje pary ref_*/opt_* i biegnie w kazdej konfiguracji przelacznikow.
# `-fires` sprawdza, ze w planie bez VAR R1 nadal dziala, wiec przy wylaczonym
# przebiegu jest tautologicznie falszywy i wylaczony.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify_value.sh)

add_test(
  NAME ${TestID}-fires
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify_fires.sh)

set_tests_properties(${TestID}-value ${TestID}-fires PROPERTIES TIMEOUT 60)

if(NOT RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES)
  set_tests_properties(
    ${TestID}-fires
    PROPERTIES
      DISABLED TRUE LABELS
      "expected_ablation_failure;requires_factor_matched_hash_timemoves")
endif()
