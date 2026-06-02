# cd build/Debug && make install && ctest -R issue113_meta_autocreate -V

# Weryfikuje automatyczne tworzenie pliku sidecar .meta (issue #113): po
# pierwszym append plik .meta musi istnieć i mieć rozmiar >16 bajtów;
# kolejne wywołanie xtrdb z komendą "meta" musi raportować nazwę pliku
# i nie zgłaszać błędu "meta file not found".

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ; \
    test -f test_db.meta ; \
    test $(stat -c%s test_db.meta) -gt 16 ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term_meta.script > out_meta.txt ; \
    grep -q 'meta: test_db.meta' out_meta.txt ; \
    ! grep -q 'meta file not found' out_meta.txt")
