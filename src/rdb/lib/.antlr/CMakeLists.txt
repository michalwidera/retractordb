cmake_minimum_required(VERSION 3.10)
project(descparser LANGUAGES CXX)

find_package(antlr4-runtime REQUIRED)

file(GLOB BUILDPARSERDSC_CPP "*.cpp")

add_library(${PROJECT_NAME} STATIC ${BUILDPARSERDSC_CPP})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} antlr4_static)
