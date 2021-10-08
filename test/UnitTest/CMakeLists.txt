cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project( regression-test )

INCLUDE_DIRECTORIES("${Boost_INCLUDE_DIR};${CMAKE_SOURCE_DIR}/src/share;${CMAKE_SOURCE_DIR}/src;${CMAKE_SOURCE_DIR}/src/compiler")

set(CMAKE_REGRESSION_DATA_DIR ${CMAKE_SOURCE_DIR}/test/Regression/Data)

set(SOURCES_QRY_TEST
  ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp
  test_qry.cpp
  main.cpp
)

set(SOURCES_COMPILER_TEST
  ${CMAKE_SOURCE_DIR}/src/compiler/compiler.cpp
  ${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
  ${CMAKE_SOURCE_DIR}/src/share/QStruct.cpp
  ${CMAKE_SOURCE_DIR}/src/share/SOperations.cpp
  test_compiler.cpp
  main.cpp
)

add_executable( test_xqry ${SOURCES_QRY_TEST} )
add_executable( test_xcompiler ${SOURCES_COMPILER_TEST} )

target_link_libraries(test_xqry ${CONAN_LIBS})
target_link_libraries(test_xcompiler ${CONAN_LIBS})

add_test( 
  NAME unittest-test-xqry
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/bin/test_xqry
)

add_test( 
  NAME unittest-test-xcompiler
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/bin/test_xcompiler
)
