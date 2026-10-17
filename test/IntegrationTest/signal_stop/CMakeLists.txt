# Zatrzymanie sygnalem (SIGTERM/SIGINT/SIGHUP) w kazdym trybie pracy i handler
# sygnalu bez logowania (#284). Pelne uzasadnienie: run.sh.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: launcher.xretractor.5~1, launcher.xretractor.7~1, launcher.xretractor.15~1, launcher.xretractor.33~1
# covers: executorsm.executorsm.16~1, executorsm.executorsm.20~1
# Typ buildu decyduje, czy komunikat SPDLOG_WARN o sygnale jest w ogole
# wkompilowany (Release wycina go progiem SPDLOG_ACTIVE_LEVEL, glowny
# CMakeLists.txt). Dlatego binarka z tego drzewa, a nie kopia z ~/.local/bin -
# ta druga bywa z innego typu buildu.
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh $<TARGET_FILE:xretractor> "${CMAKE_BUILD_TYPE}")

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 300)
