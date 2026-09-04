# make install && ctest -R simple -V

# Podstawowy test dymny arytmetyki na połączonych strumieniach. core0 (rate 0.1)
# i core1 (rate 0.2) łączone operatorem +; SELECT oblicza wyrażenia arytmetyczne
# na polach str1. Wynik porównywany z pattern-run.txt. Test -vg-run dodatkowo
# uruchamia valgrind z wykrywaniem wycieków pamięci.

# K24/H10a: przebieg wydluzony z 10 na 11 taktow. Ogon `+` wzrosl o jeden slot
# (rekord wolniejszej skladowej jest okreslony dopiero na koncu jej wlasnego
# slotu), wiec ten sam zestaw rekordow wymaga jednego taktu wiecej. Wzorzec
# pozostaje bajtowo bez zmian - zmienila sie chwila emisji, nie tresc.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten
# sam. Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera
# -f znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
add_test(
  NAME ${TestID}-vg-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    valgrind --vgdb=no --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    query.rql -m 5 -f)

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e && \
  xretractor query.rql -m 11 -f && \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt &&  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out.txt")

# Testy kompilacyjne tego katalogu, do 2026-09-04 osobny katalog
# IntegrationTest_parallel/simple. Podzial na drzewo "serial" i "parallel"
# stracil sens, gdy testy serii dostaly przestrzenie nazw i zaczely biec
# rownoczesnie; te testy i tak pracowaly na danych stad, tylko z drugiego
# CMakeLists.

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c > out-compile.txt && \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out-compile.txt")

add_test(
  NAME ${TestID}-dot
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c -d -f -t -s > out.dot && \
    ${CMAKE_COMMAND} -E compare_files pattern-dot.txt out.dot")

add_test(
  NAME ${TestID}-vg-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    valgrind --vgdb=no --error-exitcode=1 --leak-check=full
    --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../../valgrind.supp xretractor
    query.rql -c)
