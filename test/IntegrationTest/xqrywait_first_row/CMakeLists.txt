# cd build/Debug && ninja install && ctest -R xqrywait_first_row -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Pierwszy wiersz strumienia nie ma prawa zginac miedzy komenda, ktora zdejmuje
# bramke -x, a komenda, ktora zaklada kolejke odpowiedzi klienta. Oba okna tej
# drogi sa rozciagane hakami diagnostycznymi, wiec test jest deterministyczny.

# covers: qry.qry.3~1, launcher.xretractor.32~1, executorsm.executorsm.3~1, executorsm.executorsm.5~1, executorsm.executorsm.9~1
# covers: ipcserver.ipc-server.3~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
