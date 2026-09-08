# cd build/Debug make install ; ctest -R issue113_meta -V

# Weryfikuje zachowanie xtrdb przy append bez modyfikacji pól (issue #113): dwa
# rekordy wpisane do jednokolumnowej bazy, następnie lista i hexdump pliku
# binarnego porównywane z oczekiwanym wzorcem pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e && \
  rm -f test_db.meta test_db.meta.shadow && \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt &&  \
  bash ../compare.sh --ignore-eol pattern.txt out.txt")
