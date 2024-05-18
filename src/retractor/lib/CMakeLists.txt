cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(retractor LANGUAGES CXX)

add_subdirectory(rqlParser)

file(GLOB BUILDSRCS_CPP "*.cpp")
file(GLOB BUILDCOMMON_CPP "../../common/*.cpp")

add_library(${PROJECT_NAME} STATIC ${BUILDSRCS_CPP} ${BUILDCOMMON_CPP})

target_include_directories(
  ${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR} ${spdlog_INCLUDE_DIR}
                          ${fmt_INCLUDE_DIR})
