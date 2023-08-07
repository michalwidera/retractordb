cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

include_directories(AFTER ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src)

set(SOURCES_QRY_TEST ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp test_qry.cpp main.cpp)
set(SOURCES_COMPILER_TEST test_compiler.cpp main.cpp)
set(SOURCES_RDB_TEST test_rdb.cpp main.cpp)
set(SOURCES_DATAMODEL_TEST test_dataModel.cpp main.cpp)
set(SOURCES_BUFFERMANAGEMENT_TEST test_buffManagement.cpp main.cpp)
set(SOURCES_CRSMATH_TEST test_crsMath.cpp main.cpp)
set(SOURCES_EXPEVAL_TEST test_expeval.cpp main.cpp)
set(SOURCES_SOPERATIONS_TEST test_soperations.cpp main.cpp)

add_executable(test_xqry ${SOURCES_QRY_TEST})
add_executable(test_xtrdb ${SOURCES_RDB_TEST})
add_executable(test_dataModel ${SOURCES_DATAMODEL_TEST})
add_executable(test_expeval ${SOURCES_EXPEVAL_TEST})
add_executable(test_compiler ${SOURCES_COMPILER_TEST})
add_executable(test_soperations ${SOURCES_SOPERATIONS_TEST})
add_executable(test_buffManagement ${SOURCES_BUFFERMANAGEMENT_TEST})
add_executable(test_crsMath ${SOURCES_CRSMATH_TEST})

set(LIBRARY_TEST_SET boost::boost gtest::gtest spdlog::spdlog_header_only antlr4_static retractor rdb parser)

target_link_libraries(test_xqry PRIVATE ${LIBRARY_TEST_SET})
target_link_libraries(test_xtrdb PRIVATE ${LIBRARY_TEST_SET})
target_link_libraries(test_dataModel PRIVATE ${LIBRARY_TEST_SET})
target_link_libraries(test_expeval PRIVATE ${LIBRARY_TEST_SET})
target_link_libraries(test_compiler PRIVATE ${LIBRARY_TEST_SET})
target_link_libraries(test_soperations PRIVATE ${LIBRARY_TEST_SET})
target_link_libraries(test_buffManagement PRIVATE ${LIBRARY_TEST_SET})
target_link_libraries(test_crsMath PRIVATE ${LIBRARY_TEST_SET})

add_test(
  NAME ut-xqry
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

add_test(
  NAME ut-xqry-vg
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

add_test(
  NAME ut-xtrdb
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/rdb
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xtrdb)

add_test(
  NAME ut-xtrdb-vg
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/rdb
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_xtrdb)

add_test(
  NAME ut-expeval
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_expeval)

add_test(
  NAME ut-soperations
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_soperations)

add_test(
  NAME ut-soperations-vg
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_soperations)

add_test(
  NAME ut-expeval-vg
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_expeval)

add_test(
  NAME ut-compiler
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/compiler
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_compiler)

add_test(
  NAME ut-compiler-vg
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/compiler
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_compiler)

add_test(
  NAME ut-dataModel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/dataModel
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_dataModel)

add_test(
  NAME ut-buffManagement
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/dataModel
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_buffManagement)

add_test(
  NAME ut-crsMath
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/dataModel
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_crsMath)

add_test(
  NAME ut-dataModel-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/dataModel
  COMMAND sh -c "set -e ; \
    xretractor ut_example_schema.rql -c > out_compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern_compile.txt out_compile.txt")

add_test(
  NAME ut-dataModel-vg
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/dataModel
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_dataModel)

add_test(
  NAME ut-dataModel-comp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data/dataModel
  COMMAND bash -c "set -e ; xtrdb < term.script > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

set_tests_properties(ut-xtrdb ut-xqry ut-dataModel ut-dataModel-comp
                     ut-soperations PROPERTIES RUN_SERIAL TRUE)
