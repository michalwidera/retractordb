cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(retractordb)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)

enable_testing()

include_directories(
  ${Boost_INCLUDE_DIR} ${spdlog_INCLUDE_DIR} ${fmt_INCLUDE_DIR}
  ${antlr4-runtime_INCLUDE_DIR} src/include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compileCommands req. from
                                      # .vsccode/c_cpp_properties.json

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)
