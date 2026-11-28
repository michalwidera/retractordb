# Wiersz strumienia z polem tablicowym: serwer splaszcza INTEGER[3] na trzy
# WARTOSCI, a kazdy format klienta ma pokazac je w komplecie i pod wlasna nazwa.
# Drugi strumien jest tak szeroki, ze wiersz nie miesci sie w slocie kolejki
# odpowiedzi -- to nie ma prawa konczyc uslugi.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: qry.qry.5~1, launcher.xretractor.32~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Budzet czasu: subskrypcja strumienia, ktorego wiersz nie przechodzi, konczy
# sie dopiero limitem "brak danych" po stronie klienta
# (kDefaultServerNoDataTimeoutMs = 10 s).
set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 90)

# Wariant -vg-: ten sam scenariusz z programami pod valgrindem (it_add_vg_test).
# Formaty wyjscia xqry (-r, -g, -f, -p) dla wiersza z polem tablicowym.
it_add_vg_test(
  NAME
  ${TestID}-vg-run
  WORKING_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
  bash
  run.sh)
set_tests_properties(${TestID}-vg-run PROPERTIES TIMEOUT 300)
