cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(xqry LANGUAGES CXX)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

set(SOURCES_QRY qryLauncher.cpp qry.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_QRY})

target_link_libraries(${PROJECT_NAME} boost::boost spdlog::spdlog_header_only)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)
