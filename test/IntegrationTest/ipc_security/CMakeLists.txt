get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

foreach(_case lifecycle foreign-uid)
  # covers: ipcsecurity.remove-owned.2~1, lockfile.sweep.2~1, lockmanager.sweep-abandoned-resources.4~1, ipcserver.ipc-server.2~1
  # covers: ipcserver.ipc-server.13~1
  add_test(
    NAME ${TestID}-${_case}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xqry>
            ${_case})
  set_tests_properties(${TestID}-${_case} PROPERTIES TIMEOUT 120
                                                     SKIP_RETURN_CODE 77)
endforeach()

# Kontrakt startu executorsm (#521): linia PID dopiero po zasobach IPC (strace)
# i kod ENOBUFS, gdy zasobow IPC nie da sie zbudowac (unshare). Bez narzedzia -
# weryfikacja niewykonana (77).
foreach(_case order ipc-failure)
  # covers: executorsm.executorsm.1~1, executorsm.executorsm.2~1
  add_test(
    NAME ${TestID}-start-${_case}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND bash start_order.sh $<TARGET_FILE:xretractor> ${_case})
  set_tests_properties(${TestID}-start-${_case} PROPERTIES TIMEOUT 60
                                                           SKIP_RETURN_CODE 77)
endforeach()

# RUN_SERIAL: lifecycle sprawdza pozostalosci po SIGKILL, a kazda rownolegla
# instancja sprzata je przy wyjsciu (sweepAbandonedResources, blokady we
# wspolnym /tmp) - zabralaby je, zanim test zdazy sprawdzic, ze w ogole
# powstaly. Tak samo jak w leftover_sweep.
set_tests_properties(${TestID}-lifecycle PROPERTIES RUN_SERIAL TRUE)

# Drugi UID wymaga root/CAP_SETUID i mozliwosci chown. Skrypt zrzuca uprawnienia
# procesow serwera/klienta i zaklada obce obiekty jako root. Brak tych warunkow
# jest niewykonana weryfikacja, a nie zielona kontrola wlasnosci.
set_tests_properties(${TestID}-foreign-uid PROPERTIES SKIP_RETURN_CODE 77)
