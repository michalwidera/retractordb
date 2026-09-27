# make install && ctest -R unbounded_storage -V

# cmake-format: off
# D7: samo `RETENTION n` na magazynie plikowym jest bledem planu, a `STORAGE DIRECT` z planu trafia do
# magazynu. D8: strumienie rosnace na dysku bez granicy sa wymienione przy starcie i w `-c`, a klucz
# `[storage] default_retention` stawia im granice w .desc. Za mala retencja jest bledem kompilacji.
# Szczegoly w run.sh.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)
