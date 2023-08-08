cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

include_directories(AFTER ${CMAKE_SOURCE_DIR}/src/include
                    ${CMAKE_SOURCE_DIR}/src)

file(
  GLOB V_GLOB
  LIST_DIRECTORIES false
  "*.cpp")

list(FILTER V_GLOB EXCLUDE REGEX ".*test_qry\\.cpp$")

# foreach(item ${V_GLOB}) message( ${item}) endforeach()

set(LIBRARY_TEST_SET
    boost::boost
    gtest::gtest
    spdlog::spdlog_header_only
    antlr4_static
    retractor
    rdb
    parser)

foreach(item ${V_GLOB})
  get_filename_component(OUTPUT_FILE_NOEXT ${item} NAME_WLE)
  set(SOURCES_${OUTPUT_FILE_NOEXT}_TEST ${item} launch/main.cpp)
  add_executable(${OUTPUT_FILE_NOEXT} ${SOURCES_${OUTPUT_FILE_NOEXT}_TEST})
  target_link_libraries(${OUTPUT_FILE_NOEXT} PRIVATE ${LIBRARY_TEST_SET})
  add_test(
    NAME ut-${OUTPUT_FILE_NOEXT}
    WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT}
    COMMAND valgrind --error-exitcode=1 --leak-check=full
            ${CMAKE_BINARY_DIR}/test/UnitTest/${OUTPUT_FILE_NOEXT})
endforeach()

set(SOURCES_QRY_TEST ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp test_qry.cpp
                     launch/main.cpp)
add_executable(test_xqry ${SOURCES_QRY_TEST})
target_link_libraries(test_xqry PRIVATE ${LIBRARY_TEST_SET})
add_test(
  NAME ut-xqry
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_qry
  COMMAND ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

add_test(
  NAME ut-dataModel-compile
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND sh -c "set -e ; \
    xretractor ut_example_schema.rql -c > out_compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern_compile.txt out_compile.txt")

add_test(
  NAME ut-dataModel-compare
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND bash -c "set -e ; xtrdb < term.script > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

set_tests_properties(
  ut-test_rdb ut-xqry ut-test_dataModel ut-dataModel-compile
  ut-dataModel-compare ut-test_soperations PROPERTIES RUN_SERIAL TRUE)
