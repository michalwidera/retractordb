# End-to-end coverage for sharing equivalent SELECT computations across
# commutative STREAM_ADD nodes while preserving both public artifacts.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)

if(NOT RDB_OPT_SHARE_EQUIVALENT_SELECTS OR NOT RDB_OPT_COMMUTATIVE_ADD)
  set_tests_properties(
    ${TestID}-run
    PROPERTIES DISABLED TRUE LABELS
               "expected_ablation_failure;requires_commutative_select_sharing")
endif()
