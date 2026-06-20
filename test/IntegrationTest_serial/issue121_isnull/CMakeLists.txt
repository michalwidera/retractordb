# cd build/Debug && make install && ctest -R issue121_isnull -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# isnull(field) returns 1 when the field is null, 0 when it is not. Row 1: a=10,
# b=NULL  → isnull(a)=0, isnull(b)=1 Row 2: a=NULL, b=20  → isnull(a)=1,
# isnull(b)=0 Row 3: a=30, b=40    → isnull(a)=0, isnull(b)=0

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startują serwer (blokada usługi) -> RUN_SERIAL jawnie (flagi -k/-m są w
# run.sh, więc makro add_test ich nie widzi).
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
