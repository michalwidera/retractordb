# cd build/Debug && make install && ctest -R string_field_passthrough -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Pole STRING ze zrodla tekstowego BEZ cudzyslowow: wartosc ma dojsc do
# wyrazenia w SELECT, rekord ma zostac wyrownany (txt=42, k=7), a typ pola ma
# trafic do deskryptora jako STRING[8], a nie INTEGER.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
