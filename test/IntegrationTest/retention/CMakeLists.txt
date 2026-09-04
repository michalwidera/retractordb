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
