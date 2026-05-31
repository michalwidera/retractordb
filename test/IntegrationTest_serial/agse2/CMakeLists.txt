# make install && ctest -R agse2 -V

# Weryfikuje różne kombinacje operatora okna @(n,m) na strumieniu 3-polowym.
# Strumienie serial1..serial3, serial7, serial8 z oknami (1,1), (2,2), (1,2),
# (2,3), (2,4) — sprawdza wyrównanie i rate-conversion dla różnych proporcji.
# Wynik porównywany z pattern.txt via xtrdb.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    rm -f *.meta ; \
    xretractor query.rql -m 15 -v > verbose.txt ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ;  \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
