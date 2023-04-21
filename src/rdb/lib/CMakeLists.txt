cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(rdb LANGUAGES CXX)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

file(GLOB_RECURSE BUILDRDB_CC "*.cc")

add_library(${PROJECT_NAME} STATIC ${BUILDRDB_CC} ../../share/convertTypes.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR}
                                       ${spdlog_INCLUDE_DIR} ${fmt_INCLUDE_DIR})
