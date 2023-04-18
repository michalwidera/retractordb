cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(regression-test)

# these set of 3 commands will copy artifacts from UnitTest/Data into
# build/Debug/test folder. First will cleanup this build/Debug/test/Data folder
# then copy.
add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(antlr4-runtime REQUIRED)
find_package(GTest REQUIRED)

file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/Data)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_UNITTEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data)

include_directories(AFTER ${CMAKE_SOURCE_DIR}/src/include
                    ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/share)

set(SOURCES_QRY_TEST ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp test_qry.cpp main.cpp)

set(SOURCES_COMPILER_TEST
    ${CMAKE_SOURCE_DIR}/src/retractor/compiler.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/share/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/share/convertTypes.cpp
    ${CMAKE_SOURCE_DIR}/src/rdb/descriptor.cc
    ${CMAKE_SOURCE_DIR}/src/share/SOperations.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/Parser/RQLLexer.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/Parser/RQLParser.cpp
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
    ${CMAKE_SOURCE_DIR}/src/share/convertTypes.cpp
    test_rdb.cpp
    main.cpp)

set(SOURCES_DATAMODEL_TEST
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
    ${CMAKE_SOURCE_DIR}/src/share/convertTypes.cpp
    ${CMAKE_SOURCE_DIR}/src/share/SOperations.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/Parser/RQLLexer.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/Parser/RQLParser.cpp
    test_dataModel.cpp
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
    ${CMAKE_SOURCE_DIR}/src/share/convertTypes.cpp
    ${CMAKE_SOURCE_DIR}/src/share/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/share/SOperations.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/Parser/RQLLexer.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/Parser/RQLParser.cpp
    test_expeval.cpp
    main.cpp)

add_executable(test_xqry ${SOURCES_QRY_TEST})
add_executable(test_xtrdb ${SOURCES_RDB_TEST})
add_executable(test_dataModel ${SOURCES_DATAMODEL_TEST})
add_executable(test_expeval ${SOURCES_EXPEVAL_TEST})
add_executable(test_compiler ${SOURCES_COMPILER_TEST})

target_link_libraries(test_xqry boost::boost gtest::gtest antlr4_static
                      spdlog::spdlog_header_only)
target_link_libraries(test_xtrdb boost::boost gtest::gtest
                      spdlog::spdlog_header_only)
target_link_libraries(test_dataModel boost::boost gtest::gtest antlr4_static
                      spdlog::spdlog_header_only)
target_link_libraries(test_expeval boost::boost gtest::gtest antlr4_static
                      spdlog::spdlog_header_only)
target_link_libraries(test_compiler boost::boost gtest::gtest antlr4_static
                      spdlog::spdlog_header_only)

add_test(
  NAME ut-xqry
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

add_test(
  NAME ut-xtrdb
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/rdb
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xtrdb)

add_test(
  NAME ut-expeval
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_expeval)

add_test(
  NAME ut-compiler
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/compiler
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_compiler)

add_test(
  NAME ut-dataModel
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/dataModel
  COMMAND bash -c "set -e ; \
  ${CMAKE_BINARY_DIR}/test/UnitTest/test_dataModel")

add_test(
  NAME ut-dataModel-comp
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/dataModel
  COMMAND bash -c "set -e ; \
  xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

set_tests_properties(ut-xtrdb ut-xqry ut-dataModel ut-dataModel-comp
                     PROPERTIES RUN_SERIAL TRUE)
