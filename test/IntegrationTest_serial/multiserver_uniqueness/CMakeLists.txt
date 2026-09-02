# make install && ctest -R it_multiserver_uniqueness -V

# Weryfikuje magistrale xrdbbus: kolizja nazwy strumienia miedzy instancjami
# jest odmowa startu wskazujaca wlasciciela, nazwy rozlaczne startuja normalnie,
# a po smierci wlasciciela nazwa jest znowu wolna (patrz naglowek
# uniqueness.sh).
#
# Logika scenariusza jest w uniqueness.sh, a NIE w 'bash -c "...; ...; ..."', bo
# srednik w add_test() jest separatorem listy CMake i rozbilby skrypt na osobne
# argumenty.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash uniqueness.sh)

# Startuje serwery, wiec musi biec szeregowo wzgledem pozostalych testow
# serwerowych.
set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
