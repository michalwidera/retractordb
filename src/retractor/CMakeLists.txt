cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(xretractor LANGUAGES CXX)

add_subdirectory(lib)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4 REQUIRED)

set(SOURCES_RETRACTOR ${BUILDSRCS_CPP} ${BUILDCOMMON_CPP})

add_executable(${PROJECT_NAME} launcher.cpp)

target_link_libraries(
  ${PROJECT_NAME}
  boost::boost
  spdlog::spdlog_header_only
  antlr4_static
  retractor
  rdb
  descparser
  rqlparser)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)


