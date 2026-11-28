# make install && ctest -R issue113_meta_internal -V

# Weryfikuje poprawność struktury pliku metadanych .meta generowanego przez
# xretractor (issue #113). Po uruchomieniu xretractor z 5 rekordami (w tym null)
# xtrdb odczytuje sidecar i sprawdza: rozmiar nagłówka (8), rozmiar wpisu (18),
# interwał próbkowania (1/1), trzy wpisy z poprawnymi bitsetami null: entry[0]:
# count=1 bitset=0x03 (oba pola null) entry[1]: count=2 bitset=0x02 (tylko b
# null) entry[2]: count=1 bitset=0x00 (brak null)

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 60)

# Wariant -vg-: ten sam scenariusz z programami pod valgrindem (it_add_vg_test).
# Komendy meta xtrdb (cmdMeta.cpp) na sidecarze .meta z wartosciami NULL.
it_add_vg_test(
  NAME
  ${TestID}-vg-run
  WORKING_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
  bash
  run.sh)
set_tests_properties(${TestID}-vg-run PROPERTIES TIMEOUT 300)
