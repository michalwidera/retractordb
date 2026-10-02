# cd build/Debug && ninja install && ctest -R ipc_client_killed_reading -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Klient zabity kill -9 w trakcie odbioru odpowiedzi nie moze zablokowac obslugi
# komend. Klient jest zatrzymywany w stanie READING hakiem
# RDB_FAULT_CLIENT_READ_DELAY, wiec test jest deterministyczny.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
