# make install && ctest -R agse_volatile -V

# Okno AGSE nad producentem VOLATILE (bufor MEMORY). Pojemnosc bufora liczona
# przez compiler::computeRequiredCapacities() musi obejmowac caly zakres okna;
# regresja objawiala sie podstawieniem rekordu najnowszego w miejsce
# najstarszego pola okna.

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
    bash -c
    "set -e && \
    rm -f *.meta win_out && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 15 -f > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
