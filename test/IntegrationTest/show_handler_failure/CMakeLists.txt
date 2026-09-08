# Awaria handlera 'show' ma dojechac do klienta jako POWOD, a nie jako cisza.
# Pelne uzasadnienie i historia awarii: run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
