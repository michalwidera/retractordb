cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(retractordb)

enable_testing()

# set(CONAN_DISABLE_CHECK_COMPILER TRUE)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(antlr4-runtime REQUIRED)
find_package(GTest REQUIRED)

include_directories(
  ${Boost_INCLUDE_DIR} ${spdlog_INCLUDE_DIR} ${antlr4-runtime_INCLUDE_DIR}
  ${GTest_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/include
  ${CMAKE_SOURCE_DIR}/src/share)

set(CMAKE_CXX_STANDARD 20)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake) conan_basic_setup()

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)
