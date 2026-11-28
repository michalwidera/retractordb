# Weryfikuje że wygenerowany substrat współdzielony między strumieniami
# użytkownika jest poprawnie referencjonowany przez kompilator (issue #96):
# STREAM_TIMEMOVE_2_core0 musi być reużyty, a nie powielony.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor queryWithSubstrats.rql -c > out-compile.txt && \
    bash ../compare.sh pattern.txt out-compile.txt")

if(NOT RDB_OPT_DEDUP_SUBSTRATES)
  set_tests_properties(
    ${TestID}-compile
    PROPERTIES DISABLED TRUE LABELS
               "expected_ablation_failure;requires_dedup_substrates")
endif()
