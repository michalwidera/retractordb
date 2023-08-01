cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

enable_testing()

set(CMAKE_REGRESSION_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data)

file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/..
     PATTERN "CMakeLists.txt" EXCLUDE )

file(GLOB V_GLOB LIST_DIRECTORIES true "*")
foreach(item ${V_GLOB})
  if(IS_DIRECTORY ${item})
    add_subdirectory(${item})
  endif()
endforeach()

