# make install && ctest -R it_service_idle -V

# Weryfikuje tryb usługowy xretractor (systemd): start bezczynny (idle) bez
# pliku .rql oraz logowanie usługowe na stderr (format '<N>[L] treść' z
# prefiksem priorytetu sd-daemon, bez znacznika czasu). -run : tryb włączony
# flagą --service. -env : ten sam efekt włączony zmienną środowiskową
# XRETRACTOR_SERVICE=1.
#
# Logika scenariusza jest w service-idle.sh (kopiowanym do katalogu build przy
# konfiguracji), a NIE w 'bash -c "...; ...; ..."', bo średnik w add_test() jest
# separatorem listy CMake i rozbiłby skrypt na osobne argumenty.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash service-idle.sh)

add_test(
  NAME ${TestID}-env
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash service-idle.sh env)

# Logi INFO sa wkompilowane tylko poza Release (SPDLOG_ACTIVE_LEVEL, glowny
# CMakeLists.txt). W Release tryb uslugowy nie emituje markerow INFO — skrypt
# pomija wtedy asercje INFO (podaza za cichym, efektywnym Release). Przekazujemy
# efektywny stan INFO przez zmienna srodowiskowa testu.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(_rdbInfoActive 0)
else()
  set(_rdbInfoActive 1)
endif()

set_tests_properties(${TestID}-run ${TestID}-env PROPERTIES TIMEOUT 60)

# DOPISANIE, nie ustawienie: ENVIRONMENT niesie juz przestrzen nazw uruchomienia
# (RDB_NAMESPACE, TMPDIR) nadana przez makro add_test w katalogu nadrzednym, a
# set_tests_properties zastapiloby ja w calosci — test startowalby wtedy na
# zasobach wspolnych i kolidowal z sasiadem biegnacym rownoczesnie.
set_property(
  TEST ${TestID}-run ${TestID}-env
  APPEND
  PROPERTY ENVIRONMENT "RDB_INFO_ACTIVE=${_rdbInfoActive}")
