add_test(NAME st_buildrdb_cli
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/buildrdb_cli.sh
                 ${CMAKE_SOURCE_DIR})

add_test(NAME st_xplot_lifecycle
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/xplot_lifecycle.py
                 ${CMAKE_SOURCE_DIR})
set_tests_properties(st_xplot_lifecycle PROPERTIES TIMEOUT 30)
