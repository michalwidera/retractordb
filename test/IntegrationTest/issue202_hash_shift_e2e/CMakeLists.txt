# E2E regression for: (A > i) # (B > k) -> (A # B) > (i + k) when i*delta(A) ==
# k*delta(B).
#
# `matched` exercises the optimized left-hand side. `CC` spells the right-hand
# side explicitly over independent copies of the input streams.
#
# Split in two on 2026-09-06. `-shape` asserts that the rewrite fired, so it
# cannot hold with the pass off and is disabled there. `-value` asserts the
# computed result and runs in every switch configuration — a single test could
# not do both, because one `cmp` over the whole artifact pins `Val` and `Lat` at
# once. The reasoning is in the header of each script.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-shape
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify_shape.sh)

add_test(
  NAME ${TestID}-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify_value.sh)

set_tests_properties(${TestID}-shape ${TestID}-value PROPERTIES TIMEOUT 60)

if(NOT RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES)
  set_tests_properties(
    ${TestID}-shape
    PROPERTIES
      DISABLED TRUE LABELS
      "expected_ablation_failure;requires_factor_matched_hash_timemoves")
endif()
