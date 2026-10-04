# make install && ctest -R dump_nofollow -V

# Issue #277 (S-05): plik zrzutu DO DUMP otwierany przez unlink + O_EXCL |
# O_NOFOLLOW, wiec dowiazanie podstawione pod jego nazwe nie prowadzi zapisu do
# cudzego pliku. Szczegoly w verify.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    bash verify.sh > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
