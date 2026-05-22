macro(add_test)
  _add_test(${ARGV})
  cmake_parse_arguments(_at "" "NAME" "" ${ARGV})
  if(_at_NAME)
    set_tests_properties(${_at_NAME} PROPERTIES RUN_SERIAL TRUE)
  else()
    set_tests_properties(${ARGV0} PROPERTIES RUN_SERIAL TRUE)
  endif()
endmacro()

file(
  GLOB V_GLOB
  LIST_DIRECTORIES true
  "*")
foreach(item ${V_GLOB})
  if(IS_DIRECTORY ${item})
    add_subdirectory(${item})
  endif()
endforeach()
