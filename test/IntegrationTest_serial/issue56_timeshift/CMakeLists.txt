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
# jego powrocie proces juz nie istnieje. Usunieta petla odpytywala twardy /tmp
# i tak czy tak nie dowodzila konca procesu — blokade kasuje destruktor przed
# wyjsciem z main (zob. ../serverlib.sh).
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "
set -e
xretractor query-51.rql -m 11
${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out-run.txt
${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out-run.txt
")
