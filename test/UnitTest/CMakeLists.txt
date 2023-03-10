cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(regression-test)

include_directories(
  ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src/share
  ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/compiler)

set(CMAKE_REGRESSION_DATA_DIR ${CMAKE_SOURCE_DIR}/test/Regression/Data)
set(CMAKE_UNITTEST_DATA_DIR ${CMAKE_SOURCE_DIR}/test/UnitTest/Data)

set(SOURCES_QRY_TEST ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp test_qry.cpp main.cpp)

set(SOURCES_COMPILER_TEST
    ${CMAKE_SOURCE_DIR}/src/compiler/compiler.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/share/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/rdb/descriptor.cc
    ${CMAKE_SOURCE_DIR}/src/share/SOperations.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/Parser/RQLLexer.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/Parser/RQLParser.cpp
    test_compiler.cpp
    main.cpp)

set(SOURCES_RDB_TEST
    ${CMAKE_SOURCE_DIR}/src/rdb/descriptor.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/storageacc.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccfs.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccposix.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccposixprm.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccbindev.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/facctxtsrc.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/payload.cc
    test_rdb.cpp
    main.cpp)

set(SOURCES_SCHEMA_TEST
    ${CMAKE_SOURCE_DIR}/src/rdb/descriptor.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/storageacc.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccfs.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccposix.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccposixprm.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccbindev.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/facctxtsrc.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/payload.cc
    ${CMAKE_SOURCE_DIR}/src/retractor/dataModel.cpp
    ${CMAKE_SOURCE_DIR}/src/share/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/Parser/RQLLexer.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/Parser/RQLParser.cpp
    test_schema.cpp
    main.cpp)

set(SOURCES_EXPEVAL_TEST
    ${CMAKE_SOURCE_DIR}/src/rdb/descriptor.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/storageacc.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccfs.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccposix.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccposixprm.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/faccbindev.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/facctxtsrc.cc
    ${CMAKE_SOURCE_DIR}/src/rdb/payload.cc
    ${CMAKE_SOURCE_DIR}/src/retractor/dataModel.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/expressionEvaluator.cpp
    ${CMAKE_SOURCE_DIR}/src/share/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/Parser/RQLLexer.cpp
    ${CMAKE_SOURCE_DIR}/src/compiler/Parser/RQLParser.cpp
    test_expeval.cpp
    main.cpp)

add_executable(test_xqry ${SOURCES_QRY_TEST})
add_executable(test_xcompiler ${SOURCES_COMPILER_TEST})
add_executable(test_xtrdb ${SOURCES_RDB_TEST})
add_executable(test_schema ${SOURCES_SCHEMA_TEST})
add_executable(test_expeval ${SOURCES_EXPEVAL_TEST})

target_link_libraries(test_xqry ${Boost_LIBRARIES} ${GTest_LIBRARIES})
target_link_libraries(test_xcompiler ${Boost_LIBRARIES} ${GTest_LIBRARIES}
                      ${antlr4-runtime_LIBRARIES})
target_link_libraries(test_xtrdb ${Boost_LIBRARIES} ${GTest_LIBRARIES})
target_link_libraries(test_schema ${Boost_LIBRARIES} ${GTest_LIBRARIES}
                      ${antlr4-runtime_LIBRARIES})
target_link_libraries(test_expeval ${Boost_LIBRARIES} ${GTest_LIBRARIES}
                      ${antlr4-runtime_LIBRARIES})

add_test(
  NAME unittest-test-xqry
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

add_test(
  NAME unittest-test-xcompiler
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/compiler
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xcompiler)

add_test(
  NAME unittest-test-xtrdb
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/rdb
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xtrdb)

add_test(
  NAME unittest-test-expeval
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_expeval)

add_test(
  NAME unittest-test-schema
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/schema
  COMMAND bash -c "set -e ; \
  ${CMAKE_BINARY_DIR}/test/UnitTest/test_schema")

add_test(
  NAME unittest-test-schema-comp
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/schema
  COMMAND bash -c "set -e ; \
  xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

set_tests_properties(
  unittest-test-xtrdb unittest-test-xcompiler unittest-test-xqry
  unittest-test-schema unittest-test-schema-comp PROPERTIES RUN_SERIAL TRUE)
