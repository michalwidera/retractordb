add_test(NAME st_buildrdb_cli
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/buildrdb_cli.sh
                 ${CMAKE_SOURCE_DIR})

add_test(NAME st_xplot_lifecycle
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/xplot_lifecycle.py
                 ${CMAKE_SOURCE_DIR})
set_tests_properties(st_xplot_lifecycle PROPERTIES TIMEOUT 30)

# Testy API klienckiego. EXCLUDE_FROM_ALL na binarce i etykieta `api` na testach
# trzymaja je poza `ninja` i poza `ninja test`; wejsciem jest `ninja test-api`,
# ktore buduje test_api_client i uruchamia `ctest -L api`.
add_executable(test_api_client EXCLUDE_FROM_ALL api_client.cpp)
target_link_libraries(test_api_client PRIVATE RetractorDB::client)
foreach(mode fake real)
  add_test(
    NAME st_api_${mode}
    COMMAND
      python3 -B ${CMAKE_CURRENT_SOURCE_DIR}/api_clients.py ${CMAKE_SOURCE_DIR}
      $<TARGET_FILE:xqry> $<TARGET_FILE:xretractor>
      $<TARGET_FILE:test_api_client> ${mode})
  # SKIP_RETURN_CODE 77: przy braku test_api_client skrypt konczy sie 77, a
  # ctest raportuje pominiecie zamiast awarii. Bez tego gole `ctest` (tak robi
  # CI) padaloby na niezbudowanej binarce API.
  set_tests_properties(st_api_${mode} PROPERTIES TIMEOUT 60 LABELS api
                                                 SKIP_RETURN_CODE 77)
endforeach()
set_tests_properties(st_api_real PROPERTIES RUN_SERIAL TRUE)
