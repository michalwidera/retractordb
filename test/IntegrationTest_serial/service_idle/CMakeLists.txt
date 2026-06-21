# make install && ctest -R it_service_idle -V

# Weryfikuje tryb usługowy xretractor (systemd): start bezczynny (idle) bez
# pliku .rql oraz logowanie usługowe na stderr (format '<N>[L] treść' z prefiksem
# priorytetu sd-daemon, bez znacznika czasu). -run : tryb włączony flagą --service.
# -env : ten sam efekt włączony zmienną środowiskową XRETRACTOR_SERVICE=1.
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

# Testy startują serwer (blokada /tmp/xretractor_service.lock), więc muszą biec
# szeregowo. Makro add_test wykrywa to po -m/-k w argv, ale tu flagi są w
# skrypcie, dlatego ustawiamy RUN_SERIAL jawnie.
set_tests_properties(${TestID}-run ${TestID}-env PROPERTIES RUN_SERIAL TRUE
                                                            TIMEOUT 60)
