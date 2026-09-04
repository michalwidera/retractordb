# Regresja optymalizacji: (A > i) # (B > k) -> (A # B) > (i + k), gdy i*deltaA
# == k*deltaB. Przypadek niedopasowany ma nadal kompilować się do dwóch
# substratów przesunięcia.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-matched
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh matched)

add_test(
  NAME ${TestID}-unmatched
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh unmatched)

add_test(
  NAME ${TestID}-zero
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh zero)

if(NOT RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES)
  set_tests_properties(
    ${TestID}-matched ${TestID}-zero
    PROPERTIES
      DISABLED TRUE LABELS
      "expected_ablation_failure;requires_factor_matched_hash_timemoves")
endif()
