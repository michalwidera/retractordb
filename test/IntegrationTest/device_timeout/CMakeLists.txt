# make install && ctest -R device_timeout -V

# cmake-format: off
# #347: odczyt DEVICE nie czeka bez konca i nie wstrzymuje reszty silnika; opcjonalny TIMEOUT.
#   -run     scenariusze na FIFO jako kontrolowanym zrodle DEVICE (szczegoly w run.sh)
#   -vg-run  faza DEVICE pod valgrindem: poll() z terminem, niepelny rekord, EOF z --until-eof
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
