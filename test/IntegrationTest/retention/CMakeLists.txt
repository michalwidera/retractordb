# Weryfikuje operacje xtrdb na pliku z parametrem RETENTION: open z retencją (3
# rekordy, 2 pliki historyczne), purge, append, list, write modyfikujący
# konkretny rekord bez zmiany pozostałych. Wynik porównywany z pattern-run.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e && \
  rm -f test_file* && \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt &&  \
  bash ../compare.sh --ignore-eol pattern-run.txt out.txt")

# Wariant -vg-: ten sam scenariusz z programami pod valgrindem (it_add_vg_test).
# Retencja, purge i write w cmdStorage.cpp.
it_add_vg_test(
  NAME
  ${TestID}-vg-run
  WORKING_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
  bash
  -c
  "set -e && \
  rm -f test_file* && \
  ${IT_VG_SHIM}/xtrdb noprompt < term.script > out.txt &&  \
  bash ../compare.sh --ignore-eol pattern-run.txt out.txt")
set_tests_properties(${TestID}-vg-run PROPERTIES TIMEOUT 300)
