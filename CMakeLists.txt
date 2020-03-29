cmake_minimum_required (VERSION 2.5)
project (db-project)

enable_testing()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

add_subdirectory(src)
add_subdirectory(test/Regression)
add_subdirectory(test/UnitTest)
add_subdirectory(scripts)

