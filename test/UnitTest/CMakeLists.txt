cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(regression-test)

# these set of 3 commands will copy artifacts from UnitTest/Data into
# build/Debug/test folder. First will cleanup this build/Debug/test/Data folder
# then copy.

file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/Data)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_UNITTEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data)

include_directories(
  ${Boost_INCLUDE_DIR}
  ${spdlog_INCLUDE_DIR}
  ${antlr4-runtime_INCLUDE_DIR}
  ${GTest_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/src/include
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/share)

set(SOURCES_QRY_TEST ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp test_qry.cpp main.cpp)

set(SOURCES_COMPILER_TEST
    ${CMAKE_SOURCE_DIR}/src/retractor/compiler.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/share/QStruct.cpp
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

target_link_libraries(test_xqry ${Boost_LIBRARIES} ${GTest_LIBRARIES}
                      ${antlr4-runtime_LIBRARIES})
target_link_libraries(test_xtrdb ${Boost_LIBRARIES} ${GTest_LIBRARIES})
target_link_libraries(test_dataModel ${Boost_LIBRARIES} ${GTest_LIBRARIES}
                      ${antlr4-runtime_LIBRARIES})
target_link_libraries(test_expeval ${Boost_LIBRARIES} ${GTest_LIBRARIES}
                      ${antlr4-runtime_LIBRARIES})
target_link_libraries(test_compiler ${Boost_LIBRARIES} ${GTest_LIBRARIES}
                      ${antlr4-runtime_LIBRARIES})

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
