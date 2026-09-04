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

# Bez czekania na plik blokady: serwer biegnie pierwszoplanowo (-m N), wiec po
# jego powrocie proces juz nie istnieje. Usunieta petla odpytywala twardy /tmp i
# tak czy tak nie dowodzila konca procesu — blokade kasuje destruktor przed
# wyjsciem z main (zob. ../serverlib.sh).
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_BINARY_DIR})

set_tests_properties(${TestID} PROPERTIES TIMEOUT 90)
