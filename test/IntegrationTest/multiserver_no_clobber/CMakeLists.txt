# make install && ctest -R it_multiserver_no_clobber -V

# Weryfikuje, ze proba uruchomienia drugiej instancji xretractor przy
# dzialajacym serwerze nie psuje temu serwerowi kanalu IPC (patrz naglowek
# no-clobber.sh).
#
# Logika scenariusza jest w no-clobber.sh, a NIE w 'bash -c "...; ...; ..."', bo
# srednik w add_test() jest separatorem listy CMake i rozbilby skrypt na osobne
# argumenty.

# Katalog poza pula przestrzeni nazw: scenariusz bada odmowe startu drugiej
# instancji o tej samej tozsamosci. RDB_NAMESPACE zmienilaby tu przedmiot
# badania, wiec makro add_test w katalogu nadrzednym daje temu testowi
# wylacznosc (RUN_SERIAL) zamiast wlasnej przestrzeni.
set(IT_NO_NAMESPACE TRUE)

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash no-clobber.sh)

# Test startuje serwer (blokada ${TMPDIR:-/tmp}/xretractor_service.lock), wiec
# musi biec szeregowo wzgledem pozostalych testow serwerowych.
set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
