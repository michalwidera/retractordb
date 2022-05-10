cmake_minimum_required(VERSION 3.21 FATAL_ERROR )

project (retractordb)

enable_testing()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)
