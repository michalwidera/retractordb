cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

# these set of 3 commands will copy artifacts from UnitTest/Data into
# build/Debug/test folder. First will cleanup this build/Debug/test/Data folder
# then copy.
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/Data)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_UNITTEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data)

include_directories(AFTER ${CMAKE_SOURCE_DIR}/src/include
                    ${CMAKE_SOURCE_DIR}/src)

set(SOURCES_QRY_TEST ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp test_qry.cpp main.cpp)

set(SOURCES_COMPILER_TEST
    ${CMAKE_SOURCE_DIR}/src/retractor/compiler.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/SOperations.cpp
    test_compiler.cpp
    main.cpp)

set(SOURCES_RDB_TEST test_rdb.cpp main.cpp)

set(SOURCES_DATAMODEL_TEST
    ${CMAKE_SOURCE_DIR}/src/retractor/dataModel.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/SOperations.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/parser.cpp
    test_dataModel.cpp
    main.cpp)

set(SOURCES_EXPEVAL_TEST
    ${CMAKE_SOURCE_DIR}/src/retractor/dataModel.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/expressionEvaluator.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/QStruct.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/SOperations.cpp
    ${CMAKE_SOURCE_DIR}/src/retractor/parser.cpp
    test_expeval.cpp
    main.cpp)

add_executable(test_xqry ${SOURCES_QRY_TEST})
add_executable(test_xtrdb ${SOURCES_RDB_TEST})
add_executable(test_dataModel ${SOURCES_DATAMODEL_TEST})
add_executable(test_expeval ${SOURCES_EXPEVAL_TEST})
add_executable(test_compiler ${SOURCES_COMPILER_TEST})

set(LIBRARY_TEST_SET boost::boost gtest::gtest spdlog::spdlog_header_only rdb)

target_link_libraries(test_xqry ${LIBRARY_TEST_SET} antlr4_static)
target_link_libraries(test_xtrdb ${LIBRARY_TEST_SET})
target_link_libraries(test_dataModel ${LIBRARY_TEST_SET} antlr4_static parser)
target_link_libraries(test_expeval ${LIBRARY_TEST_SET} antlr4_static parser)
target_link_libraries(test_compiler ${LIBRARY_TEST_SET} antlr4_static parser)

add_test(
  NAME ut-xqry
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

add_test(
  NAME ut-xqry-vg
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

add_test(
  NAME ut-xtrdb
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/rdb
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xtrdb)

add_test(
  NAME ut-xtrdb-vg
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/rdb
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_xtrdb)

add_test(
  NAME ut-expeval
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_expeval)

add_test(
  NAME ut-expeval-vg
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_expeval)

add_test(
  NAME ut-compiler
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/compiler
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_compiler)

add_test(
  NAME ut-compiler-vg
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/compiler
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_compiler)

add_test(
  NAME ut-dataModel
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/dataModel
  COMMAND bash -c "set -e ; \
    ${CMAKE_BINARY_DIR}/test/UnitTest/test_dataModel")

add_test(
  NAME ut-dataModel-compile
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/dataModel
  COMMAND sh -c "set -e ; \
    xretractor ut_example_schema.rql -c > out_compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern_compile.txt out_compile.txt")

add_test(
  NAME ut-dataModel-vg
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/dataModel
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_dataModel)

add_test(
  NAME ut-dataModel-comp
  WORKING_DIRECTORY ${CMAKE_UNITTEST_DATA_DIR}/dataModel
  COMMAND bash -c "set -e ; xtrdb < term.script > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

set_tests_properties(ut-xtrdb ut-xqry ut-dataModel ut-dataModel-comp
                     PROPERTIES RUN_SERIAL TRUE)
