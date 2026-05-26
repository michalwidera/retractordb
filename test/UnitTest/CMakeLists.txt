cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

include_directories(AFTER ${CMAKE_SOURCE_DIR}/src/include
                    ${CMAKE_SOURCE_DIR}/src)

file(
  GLOB V_GLOB CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  "*.cpp")

list(FILTER V_GLOB EXCLUDE REGEX ".*test_qry\\.cpp$")

# foreach(item ${V_GLOB}) message( ${item}) endforeach()

set(LIBRARY_TEST_SET
    boost::boost
    gtest::gtest
    spdlog::spdlog_header_only
    magic_enum::magic_enum
    antlr4_static
    retractor
    rdb
    qry
    rqlparser
    descparser)

# launch/main.cpp kompilowany raz jako OBJECT library — reużywany przez wszystkie
# testy zamiast być kompilowany 26× oddzielnie.
add_library(test_main_obj OBJECT launch/main.cpp)
target_link_libraries(test_main_obj PRIVATE gtest::gtest spdlog::spdlog_header_only)

foreach(item ${V_GLOB})
  get_filename_component(OUTPUT_FILE_NOEXT ${item} NAME_WLE)
  string(REGEX REPLACE "^test_" "" TEST_SHORT ${OUTPUT_FILE_NOEXT})
  set(SOURCES_${OUTPUT_FILE_NOEXT}_TEST ${item})
  add_executable(${OUTPUT_FILE_NOEXT} ${SOURCES_${OUTPUT_FILE_NOEXT}_TEST}
                 $<TARGET_OBJECTS:test_main_obj>)
  target_link_libraries(${OUTPUT_FILE_NOEXT} PRIVATE ${LIBRARY_TEST_SET})
  file(MAKE_DIRECTORY
       ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT})
  if(OUTPUT_FILE_NOEXT STREQUAL "test_faccposix" OR OUTPUT_FILE_NOEXT STREQUAL
                                                    "test_faccposixshd")
    target_link_options(${OUTPUT_FILE_NOEXT} PRIVATE -Wl,--wrap=pread
                        -Wl,--wrap=write)
  endif()
  if(OUTPUT_FILE_NOEXT STREQUAL "test_dumpManager")
    target_link_options(${OUTPUT_FILE_NOEXT} PRIVATE -Wl,--wrap=lseek
                        -Wl,--wrap=write)
  endif()
  # test_uxSysTermTools: symbole uxSysTermTools dostępne przez retractor → common (PUBLIC)
  # test_formatter i test_ipc_transport: symbole dostępne przez bibliotekę qry (w LIBRARY_TEST_SET)
  add_test(
    NAME ut_${TEST_SHORT}-setup
    WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT}
    COMMAND bash -c "rm -f *.meta *.desc")
  set_tests_properties(ut_${TEST_SHORT}-setup
                       PROPERTIES FIXTURES_SETUP ut_${TEST_SHORT}-fixture)

  add_test(
    NAME ut_${TEST_SHORT}
    WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT}
    COMMAND valgrind --error-exitcode=1 --leak-check=full
            ${CMAKE_BINARY_DIR}/test/UnitTest/${OUTPUT_FILE_NOEXT})
  set_tests_properties(ut_${TEST_SHORT} PROPERTIES FIXTURES_REQUIRED
                                                   ut_${TEST_SHORT}-fixture)
endforeach()

add_executable(test_xqry test_qry.cpp $<TARGET_OBJECTS:test_main_obj>)
target_link_libraries(test_xqry PRIVATE ${LIBRARY_TEST_SET})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_qry)
add_test(
  NAME ut_xqry
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_qry
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel)
add_test(
  NAME ut_dataModel-compile
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND
    sh -c
    "set -e ; \
    rm -f core0.desc core1.desc str1.desc str2.desc str3.desc str4.desc ; \
    rm -f str1 str1.meta str1.shadow str2 str2.meta str2.shadow str3 str3.meta str3.shadow str4 str4.meta str4.shadow ; \
    rm -f out.txt out_compile.txt ; \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor ut_example_schema.rql -c > out_compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern_compile.txt out_compile.txt")

add_test(
  NAME ut_dataModel-compare
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND
    bash -c
    "set -e ; \
    rm -f str1 str1.meta str1.shadow str2 str2.meta str2.shadow str3 str3.meta str3.shadow str4 str4.meta str4.shadow ; \
    ${CMAKE_COMMAND} -E copy_if_different core0.desc.tpl core0.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different core1.desc.tpl core1.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str1.desc.tpl str1.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str2.desc.tpl str2.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str3.desc.tpl str3.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str4.desc.tpl str4.desc ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

set_tests_properties(ut_dataModel-compare
                     PROPERTIES DEPENDS "ut_dataModel-compile;ut_dataModel")

set_tests_properties(
  ut_rdb ut_xqry ut_dataModel ut_dataModel-compile ut_dataModel-compare
  ut_soperations PROPERTIES RUN_SERIAL TRUE)
