get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xqry>)
# RUN_SERIAL: test sprawdza pozostalosci po SIGKILL, a kazda rownolegla
# instancja sprzata je przy wyjsciu - zabralaby je, zanim test zdazy sprawdzic,
# ze w ogole powstaly.
set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
