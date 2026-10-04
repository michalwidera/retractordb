# ctest -R xtrdb_ref_dirs -V

# cmake-format: off
# REF z wczytanego `.desc` w xtrdb (#278): odczyt zewnetrznego zrodla dziala, zapis poza katalog
# magazynu wymaga `storage.ref_dirs` w retractor.toml, a `quitdrop` nie usuwa pliku spoza niego.
# Szczegoly w run.sh.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_BINARY_DIR}/src/rdb/xtrdb)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
