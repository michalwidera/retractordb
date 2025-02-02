cmake_minimum_required(VERSION 3.10)
enable_testing()

# message(STATUS ${CMAKE_CURRENT_BINARY_DIR}) message(STATUS
# ${CMAKE_BINARY_DIR}) message(STATUS ${CMAKE_PREFIX_PATH}) message(STATUS
# ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "Remove folder " ${CMAKE_BINARY_DIR}/test)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/test)

message(STATUS "Copy folder " ${CMAKE_CURRENT_SOURCE_DIR} " to "
               ${CMAKE_CURRENT_BINARY_DIR}/..)
file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/..
  PATTERN "CMakeLists.txt" EXCLUDE)

file(
  GLOB V_GLOB
  LIST_DIRECTORIES true
  "*")
foreach(item ${V_GLOB})
  if(IS_DIRECTORY ${item})
    add_subdirectory(${item})
  endif()
endforeach()
