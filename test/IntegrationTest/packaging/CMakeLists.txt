# make install && ctest -R it_packaging -V

# Smoke test budowania pakietów CPack (DEB/TGZ): cpack kończy się sukcesem, a
# pakiet zawiera binarium xretractor i jednostkę systemd. Logika w run.sh, by
# uniknąć pułapek ';'/'$' w add_test (zob. inne testy serwerowe).

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_CURRENT_BINARY_DIR}/pkgtest ${CMAKE_BINARY_DIR})

# Buduje pakiety (cpack stage'uje instalację) -> serial, by nie kolidować z
# innymi.
set_tests_properties(${TestID} PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
