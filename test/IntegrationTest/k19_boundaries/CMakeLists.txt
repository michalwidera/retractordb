# K19/G16: operator-tail audit at the observable boundary. The golden output
# distinguishes legitimate NULL values carried by a complete AGSE window from
# synthetic NULLs caused by reading beyond retained or not-yet-produced data.

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
    rm -f *.meta source agse agse_mirror sub_same sub2 sum3 && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 12 -f > verbose.txt && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
