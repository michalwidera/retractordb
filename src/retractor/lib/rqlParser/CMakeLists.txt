cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(rqlParser LANGUAGES CXX)

find_package(antlr4-runtime REQUIRED)

file(GLOB BUILDPARSERRQL_CPP "*.cpp")

add_library(${PROJECT_NAME} STATIC ${BUILDPARSERRQL_CPP})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} antlr4_static)
