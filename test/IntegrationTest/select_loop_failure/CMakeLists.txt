# Awaria petli renderowania xqry ma dojechac do operatora jako POWOD i wlasny
# kod wyjscia, a nie jako pusty strumien. Pelne uzasadnienie: run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: qry.qry.5~1, qry.qry.8~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
