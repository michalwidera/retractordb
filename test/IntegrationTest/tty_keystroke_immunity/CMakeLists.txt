# Bajt czekajacy na terminalu nie moze skracac przebiegu z zadeklarowanym
# budzetem slotow. Pelne uzasadnienie i historia awarii: verify.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR})
