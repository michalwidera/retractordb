# make install && ctest -R slot_schedule -V

# cmake-format: off
# #43: sloty planowane wzgledem stalej kotwicy czasu monotonicznego, takze bez --realtime.
#   -run     dryf, nadrabianie po przestoju, --realtime, reset, import ad hoc, SIGTERM w trakcie snu
#   -vg-run  dryf i nadrabianie pod valgrindem, okres 1/4 s
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

add_test(
  NAME ${TestID}-vg-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash vg.sh ${IT_VALGRIND})

set_tests_properties(${TestID}-run ${TestID}-vg-run PROPERTIES TIMEOUT 240)
