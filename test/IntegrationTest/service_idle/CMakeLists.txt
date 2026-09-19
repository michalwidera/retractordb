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
#
# Skrypt dostaje SCIEZKE do zbudowanej binarki ($<TARGET_FILE:xretractor>), a
# nie wola 'xretractor' z PATH. Reszta drzewa integracyjnego uzywa binarki
# zainstalowanej i tak ma zostac, ale te trzy testy asercjonuja markery INFO,
# ktorych Release nie kompiluje: kopia z ~/.local/bin zbudowana w Release (np.
# po kampanii ablacyjnej) kladla je z pustym stderr.txt, a objaw wygladal jak
# regresja silnika.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash service-idle.sh $<TARGET_FILE:xretractor>)

add_test(
  NAME ${TestID}-env
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash service-idle.sh $<TARGET_FILE:xretractor> env)

add_test(
  NAME ${TestID}-empty
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash service-idle.sh $<TARGET_FILE:xretractor> empty)

# Logi INFO sa wkompilowane tylko poza Release (SPDLOG_ACTIVE_LEVEL, glowny
# CMakeLists.txt). W Release tryb uslugowy nie emituje markerow INFO - skrypt
# pomija wtedy asercje INFO (podaza za cichym, efektywnym Release). Przekazujemy
# efektywny stan INFO przez zmienna srodowiskowa testu.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(_rdbInfoActive 0)
else()
  set(_rdbInfoActive 1)
endif()

set_tests_properties(${TestID}-run ${TestID}-env ${TestID}-empty
                     PROPERTIES TIMEOUT 60)

# DOPISANIE, nie ustawienie: ENVIRONMENT niesie juz przestrzen nazw uruchomienia
# (RDB_NAMESPACE, TMPDIR) nadana przez makro add_test w katalogu nadrzednym, a
# set_tests_properties zastapiloby ja w calosci - test startowalby wtedy na
# zasobach wspolnych i kolidowal z sasiadem biegnacym rownoczesnie. Prefiks
# priorytetu sd-daemon ("<6>[I] ") powstaje WYLACZNIE tam, gdzie po drugiej
# stronie stoi journald - poza systemd setupLoggerMain emituje sam "[I] ".
# Skrypt nie ma jak tego zgadnac, wiec dostaje te sama wartosc, z ktora
# kompilowal sie silnik.
set_property(
  TEST ${TestID}-run ${TestID}-env ${TestID}-empty
  APPEND
  PROPERTY ENVIRONMENT "RDB_INFO_ACTIVE=${_rdbInfoActive}"
           "RDB_SD_PREFIX=${RDB_HAS_SYSTEMD}")
