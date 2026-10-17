get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
# covers: launcher.xretractor.19~1, bus.bus.11~1, lockmanager.sweep-abandoned-resources.1~1
# covers: lockmanager.sweep-abandoned-resources.2~1, lockmanager.sweep-abandoned-resources.4~1
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xqry>)
# RUN_SERIAL: test sprawdza pozostalosci po SIGKILL, a kazda rownolegla
# instancja sprzata je przy wyjsciu - zabralaby je, zanim test zdazy sprawdzic,
# ze w ogole powstaly.
set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
