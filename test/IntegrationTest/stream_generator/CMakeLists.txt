# cd build/Debug ; ninja install ; ctest -R stream_generator -V
#
# Generator strumieni (`SELECT cells[$] STREAM cell[4] FROM cells`)
# koncem-do-konca: wynik rowny co do bitu recznie rozpisanym SELECT-om ORAZ
# zgodny z definicja zrodla. Sciezka uruchomieniowa, nie sam kompilator — patrz
# naglowek verify.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE)
