# make install && ctest -R issue167_dedup_cascaded -V

# Kontrola: kaskadowe wchłonięcia substratów. Runda 1: mysum wchłania
# STREAM_ADD_s1_s2 — aktualizuje PUSH_ID2(pair) w schemacie
# STREAM_TIMEMOVE_STREAM_ADD_s1_s2 i shifted. Runda 2: shifted wchłania
# STREAM_TIMEMOVE_STREAM_ADD_s1_s2 — aktualizuje PUSH_ID w schemacie merged.
# Wynik: merged ma pola shifted na offsecie 1, więc musi pojawić się
# PUSH_ID(merged[2]). Bez poprawki PUSH_ID w lSchema pola z shifted mapowałyby
# się na merged[0] i merged[1].

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e \
\; xretractor -c query.rql > out_compile.txt \
\; grep -F 'mysum(1/1)' out_compile.txt \
\; grep -F 'shifted(1/1)' out_compile.txt \
\; grep -F ':- PUSH_STREAM(mysum)' out_compile.txt \
\; grep -F ':- PUSH_STREAM(shifted)' out_compile.txt \
\; grep -F 'PUSH_ID(merged[1])' out_compile.txt \
\; grep -F 'PUSH_ID(merged[2])' out_compile.txt \
\; if grep -F 'STREAM_ADD_s1_s2(1/1)' out_compile.txt \; then exit 1 \; fi \
\; if grep -F 'STREAM_TIMEMOVE_STREAM_ADD_s1_s2(1/1)' out_compile.txt \; then exit 1 \; fi"
)
