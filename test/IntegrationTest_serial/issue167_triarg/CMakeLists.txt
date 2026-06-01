# make install && ctest -R issue167 -V

# Weryfikuje obsługę wieloargumentowych wyrażeń strumieniowych (issue #167).
# SUBSTRAT 'memory' powoduje, że dane substratów ADD są w pamięci (brak plików
# binarnych na dysku); substraty HASH korzystają z pliku dla stabilności przy
# dłuższych iteracjach (znane ograniczenie storage::read z CRS math). out1:
# s1+s2+s3 (lewostronnie rekurencyjny ADD, substrat pośredni) out2: (s1#s2)#s3
# (dwupoziomowy HASH z nawiasami) out3: s1+(s2#s3) (mieszane ADD + HASH) out4:
# s1+s2+s3+s4 (czteroargumentowy ADD, dwupoziomowy substrat) out5: s1#s2#s3
# (lewostronnie rekurencyjny HASH; substrat wspólny z out2) -m 30: faccmemory
# implementuje kołowy bufor — brak crashu przy HASH z retPosValue > 0.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    sh -c
    "set -e ; \
  rm -f out1 out1.desc out2 out2.desc out3 out3.desc out4 out4.desc out5 out5.desc \
        STREAM_ADD_s1_s2.desc STREAM_ADD_STREAM_ADD_s1_s2_s3.desc \
        STREAM_HASH_s1_s2.desc STREAM_HASH_s2_s3.desc \
        s1.desc s2.desc s3.desc s4.desc *.meta *.shadow ; \
  xretractor -c query.rql > out_compile.txt ; \
  xretractor query.rql -m 30 ; \
  while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out_run.txt ; \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern_compile.txt out_compile.txt ; \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern_run.txt out_run.txt")
