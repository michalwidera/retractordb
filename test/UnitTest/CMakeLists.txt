cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

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

# launch/main.cpp kompilowany raz jako OBJECT library — reużywany przez
# wszystkie testy zamiast być kompilowany 26× oddzielnie.
add_library(test_main_obj OBJECT launch/main.cpp)
target_link_libraries(test_main_obj PRIVATE gtest::gtest
                                            spdlog::spdlog_header_only)

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
  # test_uxSysTermTools: symbole uxSysTermTools dostępne przez retractor →
  # common (PUBLIC) test_formatter i test_ipcClient: symbole dostępne przez
  # bibliotekę qry (w LIBRARY_TEST_SET)
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
    COMMAND valgrind --vgdb=no --error-exitcode=1 --leak-check=full
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
  COMMAND valgrind --vgdb=no --error-exitcode=1 --leak-check=full
          ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel)
add_test(
  NAME ut_dataModel-compile
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND bash run_compile.sh ${CMAKE_BINARY_DIR})

add_test(
  NAME ut_dataModel-compare
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND bash run_compare.sh ${CMAKE_BINARY_DIR})

set_tests_properties(ut_dataModel-compare
                     PROPERTIES DEPENDS "ut_dataModel-compile;ut_dataModel")

# ut_ipcServer kasuje globalne nazwy IPC (segment, kolejka komend, muteks mapy),
# wiec nie moze biec obok testu, ktory ich uzywa.
set_tests_properties(
  ut_rdb
  ut_xqry
  ut_dataModel
  ut_dataModel-compile
  ut_dataModel-compare
  ut_soperations
  ut_ipcServer
  PROPERTIES RUN_SERIAL TRUE)
