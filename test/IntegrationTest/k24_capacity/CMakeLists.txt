# K24/P1: pojemnosc historii dla operatorow czytajacych w przod. Wzorzec
# rozroznia poprawny odczyt od rekordu all-NULL powstalego przez odczyt poza
# zachowana historia. Przed poprawka `sub3`/`sub4`/`sub5` byly w calosci
# all-NULL, a lancuch `win`/`lag`/`mix` konczyl sie przerwaniem procesu w
# storage::revRead.

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
    rm -f *.meta *.shadow source sub3 sub4 sub5 win lag mix && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 60 -f > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
