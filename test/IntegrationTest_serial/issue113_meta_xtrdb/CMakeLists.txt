# cd build/Debug && make install && ctest -R issue113_meta_xtrdb -V

# Weryfikuje że xretractor generuje poprawny plik sidecar .meta dla str_null
# (issue #113, ścieżka weryfikacji przez xtrdb): po uruchomieniu xretractor+xqry
# potwierdza istnienie i rozmiar .meta (>16B), a xtrdb raportuje poprawną
# ścieżkę "meta: temp/str_null.meta" bez błędu "meta file not found".

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Step 1: run xretractor to produce str_null stream + meta index

# Step 2: open str_null in xtrdb and invoke "meta", capture output

# Step 3: verify key properties of the meta output

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    xretractor query.rql -k -x & \
    while [ ! -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    xqry -s str_null -k -m 3 > /dev/null ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    test -f temp/str_null.meta ; \
    test $(stat -c%s temp/str_null.meta) -gt 16 ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ; \
    grep -q 'meta: temp/str_null.meta' out.txt ; \
    ! grep -q 'meta file not found' out.txt")
