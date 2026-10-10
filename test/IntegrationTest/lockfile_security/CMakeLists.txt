get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

foreach(_case lifecycle foreign-uid)
  add_test(
    NAME ${TestID}-${_case}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND python3 verify.py $<TARGET_FILE:xretractor> ${_case})
  set_tests_properties(${TestID}-${_case} PROPERTIES TIMEOUT 60)
endforeach()

# Drugi UID jest rzeczywisty; bez uprawnien do chown/setuid raportujemy brak
# weryfikacji.
set_tests_properties(${TestID}-foreign-uid PROPERTIES SKIP_RETURN_CODE 77)
