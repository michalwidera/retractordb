cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(xtrdb LANGUAGES CXX)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4 REQUIRED)

add_subdirectory(lib)

set(SOURCES_RDB
    xtrdbLauncher.cpp
    xtrdbStorageMap.cpp
    cmdDropFile.cpp
    cmdFieldAccess.cpp
    cmdMeta.cpp
    cmdOpen.cpp
    cmdPayload.cpp
    cmdStatus.cpp
    cmdStorage.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_RDB})

target_link_libraries(${PROJECT_NAME} boost::boost spdlog::spdlog_header_only
                      magic_enum::magic_enum rdb descparser common)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
