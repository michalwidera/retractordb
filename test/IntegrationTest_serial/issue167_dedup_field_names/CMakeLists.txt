# make install && ctest -R issue167_dedup_field_names -V

# Regresja: deduplikacja nie może scalać substratu z zapytaniem użytkownika,
# jeśli różnią się nazwy pól schematu, nawet gdy program i typy są zgodne.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    sh -c
    "set -e ; \
  xretractor -c query.rql > out_compile.txt ; \
  grep -F 'mysum(1/1)' out_compile.txt ; \
  grep -F 'STREAM_ADD_s1_s2(1/1)' out_compile.txt ; \
  grep -F '        mysum_0: INTEGER' out_compile.txt ; \
  grep -F '        s1_0: INTEGER' out_compile.txt ; \
  grep -F '        :- PUSH_STREAM(STREAM_ADD_s1_s2)' out_compile.txt ; \
  if grep -F '        :- PUSH_STREAM(mysum)' out_compile.txt ; then exit 1; fi"
)
