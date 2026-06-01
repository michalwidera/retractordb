# make install && ctest -R issue167_dedup_field_names -V

# Po naprawie deduplicateSubstrats (brak porownania nazw pol) deduplikacja scala
# STREAM_ADD_s1_s2 z mysum nawet gdy nazwy pol schematu sie roznia. Weryfikacja:
# out uzywa PUSH_STREAM(mysum), a STREAM_ADD_s1_s2 nie istnieje.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e \; xretractor -c query.rql > out_compile.txt \; grep -F 'mysum(1/1)' out_compile.txt \; grep -F 'out(1/1)' out_compile.txt \; grep -F ':- PUSH_STREAM(mysum)' out_compile.txt \; if grep -F 'STREAM_ADD_s1_s2(1/1)' out_compile.txt \; then exit 1 \; fi"
)
