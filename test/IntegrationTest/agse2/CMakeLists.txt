# make install && ctest -R agse2 -V

# Weryfikuje różne kombinacje operatora okna @(n,m) na strumieniu 3-polowym.
# Strumienie serial1..serial3, serial7, serial8 z oknami (1,1), (2,2), (1,2),
# (2,3), (2,4) — sprawdza wyrównanie i rate-conversion dla różnych proporcji.
# Wynik porównywany z pattern.txt via xtrdb.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten
# sam. Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera
# -f znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e && \
    rm -f *.meta && \
    xretractor query.rql -m 15 -f -v > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt &&  \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
