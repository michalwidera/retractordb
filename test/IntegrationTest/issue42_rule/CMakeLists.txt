# make install && ctest -R issue42_rule -V

# Weryfikuje mechanizm RULE (issue #42): warunkowe akcje wyzwalane na
# wartościach strumienia. Trzy reguły: DUMP z oknem -5..5 przy wartości < 15,
# DUMP z retencją 100 przy > 11, wywołanie SYSTEM przy wartości = 13 lub = 11.
# Wynik porównywany z pattern-run.txt via xtrdb.

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
xretractor query-42.rql -m 10 -f
${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out-run.txt
bash ../compare.sh --ignore-eol pattern-run.txt out-run.txt
")

# Testy kompilacyjne tego katalogu, do 2026-09-04 osobny katalog
# IntegrationTest_parallel/issue42_rule. Podzial na drzewo "serial" i "parallel"
# stracil sens, gdy testy serii dostaly przestrzenie nazw i zaczely biec
# rownoczesnie; te testy i tak pracowaly na danych stad, tylko z drugiego
# CMakeLists.

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query-42.rql -c > out.txt && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
