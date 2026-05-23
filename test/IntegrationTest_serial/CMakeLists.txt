macro(add_test)
  _add_test(${ARGV})
  cmake_parse_arguments(_at "" "NAME" "" ${ARGV})
  if(_at_NAME)
    set(_it_test_name ${_at_NAME})
  else()
    set(_it_test_name ${ARGV0})
  endif()
  set(_it_argv_str "${ARGV}")
  # Tests that start the xretractor server must not run in parallel with each
  # other: they share /tmp/xretractor_service.lock and boost shared memory.
  # Detection: -m N (run mode), -k (background daemon), xqry (IPC client),
  # workflow.sh (integration workflow), xretractor_service (lock file access).
  if(_it_argv_str MATCHES
     "[; ]-m[ ;]|[; ]-k[ ;]|xqry|workflow\\.sh|xlaunch\\.sh|xretractor_service")
    set_tests_properties(${_it_test_name} PROPERTIES RUN_SERIAL TRUE)
  endif()
  unset(_it_argv_str)
  unset(_it_test_name)
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
