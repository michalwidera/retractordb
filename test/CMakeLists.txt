cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

find_program(PROG valgrind REQUIRED)
message(STATUS "Tool check: ${PROG}")

enable_testing()

add_subdirectory(Regression)
add_subdirectory(UnitTest)
