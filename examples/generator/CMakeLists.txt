cmake_minimum_required(VERSION 3.20)
project(generator)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(
  Boost
  COMPONENTS system program_options
  REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

link_libraries(${Boost_LIBRARIES})

include_directories(${Boost_INCLUDE_DIR})
add_definitions("-DHAS_BOOST")

add_executable(xgenerator generator.cpp)
