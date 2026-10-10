get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

foreach(_case lifecycle foreign-uid)
  add_test(
    NAME ${TestID}-${_case}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xqry>
            ${_case})
  set_tests_properties(${TestID}-${_case} PROPERTIES TIMEOUT 120
                                                     SKIP_RETURN_CODE 77)
endforeach()

# Drugi UID wymaga root/CAP_SETUID i mozliwosci chown. Skrypt zrzuca uprawnienia
# procesow serwera/klienta i zaklada obce obiekty jako root. Brak tych warunkow
# jest niewykonana weryfikacja, a nie zielona kontrola wlasnosci.
set_tests_properties(${TestID}-foreign-uid PROPERTIES SKIP_RETURN_CODE 77)
