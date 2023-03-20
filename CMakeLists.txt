cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(retractordb)

enable_testing()

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(antlr4-runtime REQUIRED)
find_package(GTest REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # compileCommands req. from .vsccode/c_cpp_properties.json

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)
