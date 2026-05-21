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
    rqlparser
    descparser)

foreach(item ${V_GLOB})
  get_filename_component(OUTPUT_FILE_NOEXT ${item} NAME_WLE)
  set(SOURCES_${OUTPUT_FILE_NOEXT}_TEST ${item} launch/main.cpp)
  add_executable(${OUTPUT_FILE_NOEXT} ${SOURCES_${OUTPUT_FILE_NOEXT}_TEST})
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
  if(OUTPUT_FILE_NOEXT STREQUAL "test_uxSysTermTools")
    target_sources(${OUTPUT_FILE_NOEXT}
                   PRIVATE ${CMAKE_SOURCE_DIR}/src/common/uxSysTermTools.cpp)
  endif()
  if(OUTPUT_FILE_NOEXT STREQUAL "test_formatter")
    target_sources(${OUTPUT_FILE_NOEXT}
                   PRIVATE ${CMAKE_SOURCE_DIR}/src/qry/formatters.cpp)
  endif()
  if(OUTPUT_FILE_NOEXT STREQUAL "test_ipc_transport")
    target_sources(${OUTPUT_FILE_NOEXT}
                   PRIVATE ${CMAKE_SOURCE_DIR}/src/qry/ipc_transport.cpp)
  endif()
  add_test(
    NAME ut-${OUTPUT_FILE_NOEXT}-setup
    WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT}
    COMMAND bash -c "rm -f *.meta *.desc")
  set_tests_properties(
    ut-${OUTPUT_FILE_NOEXT}-setup PROPERTIES FIXTURES_SETUP
                                             ut-${OUTPUT_FILE_NOEXT}-fixture)

  add_test(
    NAME ut-${OUTPUT_FILE_NOEXT}
    WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT}
    COMMAND valgrind --error-exitcode=1 --leak-check=full
            ${CMAKE_BINARY_DIR}/test/UnitTest/${OUTPUT_FILE_NOEXT})
  set_tests_properties(
    ut-${OUTPUT_FILE_NOEXT} PROPERTIES FIXTURES_REQUIRED
                                       ut-${OUTPUT_FILE_NOEXT}-fixture)
endforeach()

set(SOURCES_QRY_TEST
    ${CMAKE_SOURCE_DIR}/src/qry/qry.cpp
    ${CMAKE_SOURCE_DIR}/src/qry/ipc_transport.cpp
    ${CMAKE_SOURCE_DIR}/src/qry/formatters.cpp test_qry.cpp launch/main.cpp)
add_executable(test_xqry ${SOURCES_QRY_TEST})
target_link_libraries(test_xqry PRIVATE ${LIBRARY_TEST_SET})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_qry)
add_test(
  NAME ut-xqry
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_qry
  COMMAND valgrind --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel)
add_test(
  NAME ut-dataModel-compile
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
  NAME ut-dataModel-compare
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND
    bash -c "set -e ; \
    rm -f str1 str1.meta str1.shadow str2 str2.meta str2.shadow str3 str3.meta str3.shadow str4 str4.meta str4.shadow ; \
    ${CMAKE_COMMAND} -E copy_if_different core0.desc.tpl core0.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different core1.desc.tpl core1.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str1.desc.tpl str1.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str2.desc.tpl str2.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str3.desc.tpl str3.desc ; \
    ${CMAKE_COMMAND} -E copy_if_different str4.desc.tpl str4.desc ; \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

set_tests_properties(
  ut-dataModel-compare PROPERTIES DEPENDS
                                  "ut-dataModel-compile;ut-test_dataModel")

set_tests_properties(
  ut-test_rdb ut-xqry ut-test_dataModel ut-dataModel-compile
  ut-dataModel-compare ut-test_soperations PROPERTIES RUN_SERIAL TRUE)
