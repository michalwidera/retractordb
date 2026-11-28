# make install && ctest -R slot_schedule -V

# cmake-format: off
# #43: sloty planowane wzgledem stalej kotwicy czasu monotonicznego, takze bez --realtime.
#   -run     dryf, nadrabianie po przestoju, --realtime, reset, import ad hoc, SIGTERM w trakcie snu
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: launcher.xretractor.15~1, launcher.xretractor.36~1, executorsm.executorsm.4~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 240)
