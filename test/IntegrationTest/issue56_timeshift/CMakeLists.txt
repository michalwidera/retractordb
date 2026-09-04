# make install && ctest -R issue56_timeshift -V

# Weryfikuje operator filtra (> 5) na połączonych strumieniach (issue #57).
# core0 + core1 z filtrem > 5 — tylko rekordy spełniające warunek trafiają do
# str1. Wynik porównywany z pattern-run.txt via xtrdb.

# K24/H10a: przebieg wydluzony z 10 na 11 taktow. Ogon `+` wzrosl o jeden slot
# (rekord wolniejszej skladowej jest okreslony dopiero na koncu jej wlasnego
# slotu), wiec ten sam zestaw rekordow wymaga jednego taktu wiecej. Wzorzec
# pozostaje bajtowo bez zmian - zmienila sie chwila emisji, nie tresc.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Bez czekania na plik blokady: serwer biegnie pierwszoplanowo (-m N), wiec po
# jego powrocie proces juz nie istnieje. Usunieta petla odpytywala twardy /tmp i
# tak czy tak nie dowodzila konca procesu — blokade kasuje destruktor przed
# wyjsciem z main (zob. ../serverlib.sh). Flaga -f (--no-clock) zdejmuje
# czekanie na zegar scienny; os czasu planu, wyrownanie slotow i ogon zostaja
# bez zmian, wiec artefakt jest bajtowo ten sam. Rownosc obu sciezek pilnuje
# it_noclock_offline. UWAGA: w trybie -c litera -f znaczy 'fields' w wyjsciu DOT
# — do wywolan kompilacyjnych jej NIE dodawac.
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "
set -e
xretractor query-51.rql -m 11 -f
${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out-run.txt
${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out-run.txt
")

# Testy kompilacyjne tego katalogu, do 2026-09-04 osobny katalog
# IntegrationTest_parallel/issue56_timeshift. Podzial na drzewo "serial" i
# "parallel" stracil sens, gdy testy serii dostaly przestrzenie nazw i zaczely
# biec rownoczesnie; te testy i tak pracowaly na danych stad, tylko z drugiego
# CMakeLists.

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query-51.rql -c > out.txt && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
