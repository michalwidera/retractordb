# make install && ctest -R xqry_server_silent -V

# cmake-format: off
# `xqry -s -m N` przy serwerze zamrozonym (SIGSTOP) w trakcie odczytu: klient ma sie zakonczyc kodem
# roznym od zera i nazwac powod na stderr. Do 2026-09-27 konczyl sie zerem, jesli dostal choc jeden
# wiersz (szczegoly w run.sh).
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
