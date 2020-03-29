cmake_minimum_required(VERSION 2.8 FATAL_ERROR )

project( database-test )

enable_testing()

set( CMAKE_CURRENT_BINARY_DIR test/Scripts)
set( CMAKE_CURRENT_DATA_DIR test/DataSet)

add_test( 
  NAME compiler-test-on-query
  WORKING_DIRECTORY ${CMAKE_CURRENT_DATA_DIR}
  COMMAND xcompiler -q query-lnx.txt 
)

add_test( 
  NAME compiler-test-on-dumper 
  WORKING_DIRECTORY ${CMAKE_CURRENT_DATA_DIR}
  COMMAND sh -c "xdumper > out.txt"
)

add_test(
  NAME compare_outputs 
  WORKING_DIRECTORY ${CMAKE_CURRENT_DATA_DIR}
  COMMAND ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt
)

add_test(
  NAME dumper-test
  WORKING_DIRECTORY ${CMAKE_CURRENT_DATA_DIR}
  COMMAND xdumper -d
)

add_test( 
  NAME retractor-waterfall
  WORKING_DIRECTORY ${CMAKE_CURRENT_DATA_DIR}
  COMMAND xretractor -m 20 --waterfall
)

add_test( 
  NAME workflow-test
  WORKING_DIRECTORY ${CMAKE_CURRENT_DATA_DIR}
  COMMAND test-workflow.sh query-lnx.txt
)

add_test( 
  NAME workflow-all-operators
  WORKING_DIRECTORY ${CMAKE_CURRENT_DATA_DIR}
  COMMAND test-all-operators.sh query-all.txt
)

