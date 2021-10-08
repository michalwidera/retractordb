cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project (retractordb-test)

enable_testing()

add_subdirectory(Regression)
add_subdirectory(UnitTest)
