# make install && ctest -R agse3 -V

# Weryfikuje operator @(n,m) przy wolniejszym wyjściu niż źródło: źródło rate
# 0.1, okna @(3,2), @(3,3), @(3,-3) — strumienie serial4, serial5, serial6.
# Sprawdza poprawność rate-conversion gdy output rate < input rate. Wynik
# porównywany z pattern.txt via xtrdb.

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
    rm -f *.meta *.desc serial4 serial5 serial6 core && \
    xretractor query.rql -m 15 -f -v > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt &&  \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
