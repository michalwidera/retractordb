# Regresja optymalizacji: (A > i) # (B > k) -> (A # B) > (i + k), gdy i*deltaA
# == k*deltaB. Przypadek niedopasowany ma nadal kompilować się do dwóch
# substratów przesunięcia.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")

add_test(
  NAME ${TestID}-matched
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xretractor matched.rql -c > matched.out ; \
    grep -F 'matched(1/15)' matched.out ; \
    grep -F ':- PUSH_STREAM(STREAM_HASH_A_B)' matched.out ; \
    grep -F ':- STREAM_TIMEMOVE(3)' matched.out ; \
    grep -F 'STREAM_HASH_A_B(1/15)' matched.out ; \
    if grep -F 'STREAM_TIMEMOVE_A(' matched.out ; then exit 1 ; fi ; \
    if grep -F 'STREAM_TIMEMOVE_B(' matched.out ; then exit 1 ; fi")

add_test(
  NAME ${TestID}-unmatched
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xretractor unmatched.rql -c > unmatched.out ; \
    grep -F 'unmatched(1/15)' unmatched.out ; \
    grep -F ':- PUSH_STREAM(STREAM_TIMEMOVE_A)' unmatched.out ; \
    grep -F ':- PUSH_STREAM(STREAM_TIMEMOVE_B)' unmatched.out ; \
    grep -F 'STREAM_TIMEMOVE_A(1/10)' unmatched.out ; \
    grep -F 'STREAM_TIMEMOVE_B(1/5)' unmatched.out ; \
    if grep -F 'STREAM_HASH_A_B(' unmatched.out ; then exit 1 ; fi")

add_test(
  NAME ${TestID}-zero
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    xretractor zero.rql -c > zero.out ; \
    grep -F 'zero(1/20)' zero.out ; \
    grep -F ':- PUSH_STREAM(STREAM_HASH_A_B)' zero.out ; \
    grep -F 'STREAM_HASH_A_B(1/20)' zero.out ; \
    if grep -F 'STREAM_TIMEMOVE(' zero.out ; then exit 1 ; fi")
