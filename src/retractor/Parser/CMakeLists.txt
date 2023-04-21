cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(parser LANGUAGES CXX)

find_package(antlr4-runtime REQUIRED)

file(GLOB BUILDPARSER_CPP "*.cpp")

add_library(${PROJECT_NAME} STATIC ${BUILDPARSER_CPP})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} antlr4_static)