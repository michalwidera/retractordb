# make install && ctest -R it_service_delivery -V

# Weryfikuje sciezke E3: `xretractor plan.rql` przy zywej usludze nadpisuje jej
# plik zapytan i zleca restart, a plan kolidujacy z instancja INNA niz docelowa
# jest odrzucany przed nadpisaniem czegokolwiek. Uzasadnienie kazdego kroku jest
# w naglowku delivery.sh.
#
# Logika scenariusza jest w delivery.sh (kopiowanym do katalogu build przy
# konfiguracji), a NIE w 'bash -c "...; ...; ..."', bo srednik w add_test() jest
# separatorem listy CMake i rozbilby skrypt na osobne argumenty.

# Katalog poza pula przestrzeni nazw: przedmiotem badania jest tozsamosc
# GLOBALNA maszyny -- stala nazwa instancji uslugowej ("service") i plik jej
# blokady, do ktorego test dopisuje atrape metadanych systemd. RDB_NAMESPACE
# nadpisalaby te nazwe wlasna, wiec makro add_test w katalogu nadrzednym daje
# temu testowi wylacznosc (RUN_SERIAL) zamiast przestrzeni nazw.
set(IT_NO_NAMESPACE TRUE)

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash delivery.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)
