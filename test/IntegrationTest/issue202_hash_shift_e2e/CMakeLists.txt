# E2E regression for: (A > i) # (B > k) -> (A # B) > (i + k) when i*delta(A) ==
# k*delta(B).
#
# `matched` exercises the optimized left-hand side. `CC` spells the right-hand
# side explicitly over independent copies of the input streams. The script
# compares their stored payloads and metadata, then checks the values against a
# formula-derived reference.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)

if(NOT RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES)
  set_tests_properties(
    ${TestID}-run
    PROPERTIES
      DISABLED TRUE LABELS
      "expected_ablation_failure;requires_factor_matched_hash_timemoves")
endif()
