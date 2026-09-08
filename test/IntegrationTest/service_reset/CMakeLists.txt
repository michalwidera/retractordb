# make install && ctest -R it_service_reset -V

# Weryfikuje przeladowanie calego planu dzialajacej uslugi jedna komenda (`xqry
# --reset plan.rql --server service`): przyjecie planu przez instancje
# bezczynna, odmowe planu wadliwego bez utraty planu dzialajacego, podmiane nazw
# strumieni na magistrali, powrot do stanu zerowego oraz pojedynczosc uslugi.
# Uzasadnienie kazdego kroku jest w naglowku reset.sh.
#
# Logika scenariusza jest w reset.sh (kopiowanym do katalogu build przy
# konfiguracji), a NIE w 'bash -c "...; ...; ..."', bo srednik w add_test() jest
# separatorem listy CMake i rozbilby skrypt na osobne argumenty.

# Katalog poza pula przestrzeni nazw: przedmiotem badania jest tozsamosc
# GLOBALNA maszyny — stala nazwa instancji uslugowej ("service") i to, ze taka
# instancja jest tylko jedna. RDB_NAMESPACE nadpisalaby te nazwe wlasna, wiec
# makro add_test w katalogu nadrzednym daje temu testowi wylacznosc (RUN_SERIAL)
# zamiast przestrzeni nazw.
set(IT_NO_NAMESPACE TRUE)

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash reset.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)
