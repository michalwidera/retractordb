cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(xretractor LANGUAGES CXX)

add_subdirectory(lib)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4 REQUIRED)

set(SOURCES_RETRACTOR launcher.cpp lockManager.cpp ${BUILDSRCS_CPP}
                      ${BUILDCOMMON_CPP})

add_executable(${PROJECT_NAME} ${SOURCES_RETRACTOR})

target_link_libraries(
  ${PROJECT_NAME}
  boost::boost
  spdlog::spdlog_header_only
  magic_enum::magic_enum
  antlr4_static
  retractor
  rdb
  descparser
  rqlparser)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
