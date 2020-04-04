cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project (retractordb)

enable_testing()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)

