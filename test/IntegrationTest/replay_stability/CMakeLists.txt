# Stabilnosc odtworzenia — twierdzenie z sekcji Exactness and Replay Stability
# artykulu: ta sama rewizja, te same wejscia, dwa przebiegi, artefakty
# identyczne co do bajtu poza 8-bajtowym znacznikiem utworzenia w naglowku
# .meta.
#
# Dotad sprawdzala to wylacznie reczna kampania K18 w rdb-experiment. Test jest
# szeregowy, bo oba przebiegi pisza do tego samego katalogu roboczego.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
