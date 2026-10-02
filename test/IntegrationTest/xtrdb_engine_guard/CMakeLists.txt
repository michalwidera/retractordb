# make install && ctest -R it_xtrdb_engine_guard -V

# Straznik "silnik dziala" w xtrdb (issue #253): xtrdb ma odmowic pracy przy
# zywej instancji nazwanej i przy instancji z blokada w paths.lock_dir.
# Scenariusz jest w guard.sh.
#
# Katalog poza pula przestrzeni nazw: scenariusz sam nadaje instancjom nazwy
# (alfa, beta) i sam ustawia TMPDIR oraz katalog blokad, bo wlasnie one sa
# przedmiotem badania. Makro add_test w katalogu nadrzednym daje mu wiec
# wylacznosc (RUN_SERIAL) zamiast wlasnej przestrzeni.
set(IT_NO_NAMESPACE TRUE)

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash guard.sh)

set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
