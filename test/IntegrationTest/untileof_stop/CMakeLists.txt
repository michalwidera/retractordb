# cd build/Debug && ninja install && ctest -R untileof_stop -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Pozycja 6 z paper-arXiv/debs/done/requested.md: brak trybu "licz do konca
# wejscia". Test porownuje artefakt przebiegu z --until-eof z przebiegiem o
# recznie dobranym -m i sprawdza, ze bez tego trybu zrodlo nadal sie zawija.

# covers: launcher.xretractor.38~1, datamodel.data-model.14~1, executorsm.executorsm.7~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)

# covers: plansource.check-declared-sources.3~2, facctxtsrc.text-source-ro.16~1, faccbindev.binary-device-ro.14~1
add_test(
  NAME ${TestID}-missing-sources
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 missing_sources.py $<TARGET_FILE:xretractor>)
set_tests_properties(${TestID}-missing-sources PROPERTIES TIMEOUT 60)

# Wpis logu o wyczerpanym zrodle ma poziom INFO, ktory Release wycina (jak w
# service_idle).
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(_rdbInfoActive 0)
else()
  set(_rdbInfoActive 1)
endif()
set_property(
  TEST ${TestID}-run
  APPEND
  PROPERTY ENVIRONMENT "RDB_INFO_ACTIVE=${_rdbInfoActive}")
