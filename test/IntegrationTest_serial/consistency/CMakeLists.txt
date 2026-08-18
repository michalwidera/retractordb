# make install && ctest -R consistency -V

# Weryfikuje spójność odczytu: dwa strumienie (str1, str2) czytają to samo
# źródło core0; str3 = str2 - str1 powinno być stale równe 100. Każda inna
# wartość oznacza brak spójności odczytu z jednego pliku przez dwa strumienie.
# Sprawdza też wyjście kompilatora (-c) względem pattern_compile.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
# Bez czekania na plik blokady: serwer biegnie pierwszoplanowo (-m N), wiec po
# jego powrocie proces juz nie istnieje. Usunieta petla odpytywala twardy /tmp i
# tak czy tak nie dowodzila konca procesu — blokade kasuje destruktor przed
# wyjsciem z main (zob. ../serverlib.sh).
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    sh -c
    "set -e ; \
  rm -f *.meta ; \
  xretractor -c query-consitency.rql > out_compile.txt ; \
  xretractor query-consitency.rql -m 5 -v > verbose.txt ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out_xtrdb.txt ; \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern_compile.txt out_compile.txt ; \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern_xtrdb.txt out_xtrdb.txt"
)
