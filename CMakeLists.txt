cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(
  retractordb
  VERSION 0.1.1
  LANGUAGES CXX)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)
add_definitions(-DHAS_BOOST)

set(CMAKE_CXX_FLAGS "-fuse-ld=mold -std=c++23")
find_package(Boost CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(antlr4-runtime CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(antlr4 CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

find_program(PROG_VALGRIND valgrind REQUIRED)
message(STATUS "Required tool: ${PROG_VALGRIND}")

if(DEFINED antlr4_PACKAGE_FOLDER_DEBUG)
  set(antlr4Path ${antlr4_PACKAGE_FOLDER_DEBUG})
else()
  set(antlr4Path ${antlr4_PACKAGE_FOLDER_RELEASE})
endif()

if(DEFINED openjdk_BIN_DIRS_DEBUG)
  set(javaPath ${openjdk_BIN_DIRS_DEBUG})
else()
  set(javaPath ${openjdk_BIN_DIRS_RELEASE})
endif()

# message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}") message(STATUS
# "antlr4Path: ${antlr4Path}") message(STATUS "javaPath: ${javaPath}")

enable_testing()

include_directories(
  ${Boost_INCLUDE_DIR}
  ${Boost_LIBRARY_DIRS}
  ${spdlog_INCLUDE_DIR}
  ${fmt_INCLUDE_DIR}
  ${magic_enum_INCLUDE_DIR}
  ${antlr4-runtime_INCLUDE_DIR}
  src/include)

link_directories(${Boost_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compileCommands req. from
                                      # .vsccode/c_cpp_properties.json

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)

install(
  DIRECTORY ${CMAKE_INSTALL_BINDIR}
  DESTINATION $ENV{HOME}/.local
  FILES_MATCHING
  PATTERN
    "*"
    PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE)

include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME "retractordb")
set(CPACK_PACKAGE_VERSION
    "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
)
set(CPACK_PACKAGE_VENDOR "Michal Widera")
set(CPACK_PACKAGE_CONTACT "michal@widera.com.pl")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RetractorDB")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-source-${CMAKE_SYSTEM_NAME}"
)
set(CPACK_SOURCE_IGNORE_FILES
    "/build/;/CMakeFiles/;/CMakeCache.txt;/bin/;/src/UML/;/.vscode/;/scripts/.vim/;/scripts/dev-test/;/scripts/sublime/;/.circleci/;/.gitignore;/.gitattributes;/CMakeUserPresets.json;/examples/;/.git/;/test/;/embold.yaml;~$;${CPACK_SOURCE_IGNORE_FILES}"
)
include(CPack) # include after setting variables above to take effect
