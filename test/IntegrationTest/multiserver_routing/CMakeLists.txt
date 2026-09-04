# make install && ctest -R it_multiserver_routing -V

# Weryfikuje routing automatyczny w xqry (etap 2c): klient sam znajduje
# wlasciciela strumienia przez magistrale xrdbbus, bez podawania --server i bez
# odpytywania serwerow (patrz naglowek routing.sh).
#
# Logika scenariusza jest w routing.sh, a NIE w 'bash -c "...; ...; ..."', bo
# srednik w add_test() jest separatorem listy CMake i rozbilby skrypt na osobne
# argumenty.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash routing.sh)

# Startuje serwery, wiec musi biec szeregowo wzgledem pozostalych testow
# serwerowych.
set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
