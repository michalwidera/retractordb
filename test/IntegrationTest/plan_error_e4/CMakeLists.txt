# K2 kompilatora i K3 ad hoc: odmowa nie konczy dzialajacego serwera (#474).
get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
# covers: executorsm.executorsm.13~1, executorsm.executorsm.17~1
add_test(
  NAME it_${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)
set_tests_properties(it_${TestID}-run PROPERTIES TIMEOUT 60)
