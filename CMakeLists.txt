cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(retractordb)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)
add_definitions(-DHAS_BOOST)

set(CMAKE_CXX_FLAGS "-fuse-ld=mold")

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(antlr4-runtime REQUIRED)
find_package(GTest REQUIRED)
find_package(antlr4)

find_program(PROG_VALGRIND valgrind REQUIRED)
message(STATUS "Required tool: ${PROG_VALGRIND}")

if(DEFINED antlr4_PACKAGE_FOLDER_DEBUG)
  set(antlr4Path ${antlr4_PACKAGE_FOLDER_DEBUG})
else()
  set(antlr4Path ${antlr4_PACKAGE_FOLDER_RELEASE})
endif()

if(DEFINED openjdk_BIN_DIRS_DEBUG)
  set(javaPath ${openjdk_BIN_DIRS_DEBUG})
else()
  set(javaPath ${openjdk_BIN_DIRS_RELEASE})
endif()

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "antlr4Path: ${antlr4Path}")
message(STATUS "javaPath: ${javaPath}")

enable_testing()

include_directories(
  ${Boost_INCLUDE_DIR} ${spdlog_INCLUDE_DIR} ${fmt_INCLUDE_DIR}
  ${antlr4-runtime_INCLUDE_DIR} src/include)

link_directories(${Boost_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compileCommands req. from
                                      # .vsccode/c_cpp_properties.json

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)
