get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
# covers: launcher.xretractor.19~1, launcher.xretractor.27~1, launcher.xretractor.30~1, ipcserver.ipc-server.1~1
# covers: lockmanager.flock-service-guard.4~1, lockmanager.sweep-abandoned-resources.2~2
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xqry>)
# cmake-format: off
# Test oblal raz w trzech pelnych przebiegach `ctest -R it_ -j 4` (2026-09-22); przyczyna pozostaje
# NIEUSTALONA. Dlaczego nie ma RUN_SERIAL, co wykluczono i co zrobic przy nastepnej awarii - README.md
# w tym katalogu.
#
# TIMEOUT 120, a nie 60: wlasne limity verify.py sumuja sie w najgorszym razie do 60 s
# (20 s na `--hello -w` plus cztery po 10 s), wiec przy progu 60 s CTest zabilby test ZANIM
# zdazylby wypisac diagnoze - czyli dokladnie w przypadku, dla ktorego ta diagnoza powstala.
# cmake-format: on
set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)
