cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(rdb LANGUAGES CXX)

file(GLOB_RECURSE BUILDRDB_CC "*.cc")

add_library(${PROJECT_NAME} STATIC ${BUILDRDB_CC})

target_include_directories(
  ${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR} ${spdlog_INCLUDE_DIR}
                          ${fmt_INCLUDE_DIR})
