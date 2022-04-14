cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project (retractordb)

enable_testing()

SET(CONAN_DISABLE_CHECK_COMPILER TRUE)

SET (CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)
