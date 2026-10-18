# Blad krytyczny konczy proces czysto (EXIT_FAILURE), a nie SIGSEGV-em ani
# SIGABRT-em ani zawieszeniem, a wyjatek poza granica handlera komendy daje
# klientowi powod i nie konczy serwera. Uzasadnienie pieciu sprawdzanych sciezek
# stoi w run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: launcher.xretractor.17~1, executorsm.executorsm.19~1, ipcserver.ipc-server.14~1, ipcserver.ipc-server.4~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 90)
