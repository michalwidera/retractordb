# make install && ctest -R it_multiserver_named -V

# Weryfikuje rezim wieloserwerowy: dwie instancje xretractor o roznych nazwach
# (--name) pracuja rownoczesnie, kazda z wlasnym obszarem IPC, wlasna blokada i
# wlasnym planem (patrz naglowek named.sh).
#
# Logika scenariusza jest w named.sh, a NIE w 'bash -c "...; ...; ..."', bo
# srednik w add_test() jest separatorem listy CMake i rozbilby skrypt na osobne
# argumenty.

# Katalog poza pula przestrzeni nazw: scenariusz sam nadaje instancjom nazwy
# (alfa, beta) i sprawdza sciezki ich plikow blokady. RDB_NAMESPACE zmienilaby
# tu przedmiot badania, wiec makro add_test w katalogu nadrzednym daje temu
# testowi wylacznosc (RUN_SERIAL) zamiast wlasnej przestrzeni.
set(IT_NO_NAMESPACE TRUE)

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash named.sh)

# Startuje serwery, wiec musi biec szeregowo wzgledem pozostalych testow
# serwerowych.
set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
