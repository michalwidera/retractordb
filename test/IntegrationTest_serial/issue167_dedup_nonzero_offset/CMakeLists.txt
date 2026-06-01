# make install && ctest -R issue167_dedup_nonzero_offset -V

# Kontrola: deduplikacja poprawnie aktualizuje tokeny PUSH_ID w lSchema
# konsumenta gdy wchłaniany substrat jest drugim argumentem STREAM_ADD
# (offset niezerowy w buforze wejściowym merged).
#
# Scenariusz:
#   SELECT * STREAM mysum  FROM s1+s2
#   SELECT * STREAM merged FROM s3+(s1+s2)
#
# Kompilator tworzy substrat STREAM_ADD_s1_s2; buildOutputSchema(STREAM_ADD)
# zapisuje w merged.lSchema tokeny PUSH_ID(STREAM_ADD_s1_s2, N).
# deduplicateSubstrats() wchłania substrat do mysum i musi przepisać te
# tokeny na PUSH_ID(mysum, N), bo inaczej localizeFieldOffsets() nie znajdzie
# klucza w offsetMap i domyślnie użyje offsetu 0 (offset s3, nie mysum).
#
# Pokrycie kodu: ścieżka PUSH_ID w nowym bloku aktualizacji lSchema
# w deduplicateSubstrats() — compiler.cpp.
#
# Weryfikacja: PUSH_ID(merged[2]) pojawia się tylko gdy mysum trafia
# na offset 1 (po s3), a nie na błędny offset 0.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e \
\; xretractor -c query.rql > out_compile.txt \
\; grep -F 'merged(1/1)' out_compile.txt \
\; grep -F ':- PUSH_STREAM(mysum)' out_compile.txt \
\; grep -F 'PUSH_ID(merged[1])' out_compile.txt \
\; grep -F 'PUSH_ID(merged[2])' out_compile.txt \
\; if grep -F 'STREAM_ADD_s1_s2(1/1)' out_compile.txt \; then exit 1 \; fi"
)
