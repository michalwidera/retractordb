cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(xtrdb LANGUAGES CXX)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

file(GLOB_RECURSE BUILDRDB_CC "*.cc")

add_library(rdb STATIC ${BUILDRDB_CC} ../share/convertTypes.cpp)

target_include_directories(rdb PRIVATE ${Boost_INCLUDE_DIR}
                                       ${spdlog_INCLUDE_DIR} ${fmt_INCLUDE_DIR})

set(SOURCES_RDB main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_RDB})

target_link_libraries(${PROJECT_NAME} PUBLIC boost::boost
                                             spdlog::spdlog_header_only rdb)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)
