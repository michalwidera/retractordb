if(NOT DEFINED SOURCE_DIR
   OR NOT DEFINED CLANG_TIDY
   OR NOT DEFINED COMPDB_DIR)
  message(FATAL_ERROR "SOURCE_DIR, CLANG_TIDY and COMPDB_DIR must be set")
endif()

file(GLOB_RECURSE ctidy_sources "${SOURCE_DIR}/src/*.cpp"
     "${SOURCE_DIR}/src/*.cc")
list(FILTER ctidy_sources EXCLUDE REGEX "[/\\\\]\\.antlr[/\\\\]")
set(ctidy_source_list "${COMPDB_DIR}/ctidy_sources.txt")
file(WRITE "${ctidy_source_list}" "")
foreach(ctidy_source IN LISTS ctidy_sources)
  file(APPEND "${ctidy_source_list}" "${ctidy_source}\n")
endforeach()

execute_process(
  COMMAND xargs -r -P 4 "${CLANG_TIDY}" -quiet "-p=${COMPDB_DIR}"
  WORKING_DIRECTORY "${SOURCE_DIR}"
  INPUT_FILE "${ctidy_source_list}"
  RESULT_VARIABLE ctidy_result
  OUTPUT_VARIABLE ctidy_stdout
  ERROR_VARIABLE ctidy_stderr)

set(ctidy_output "${ctidy_stdout}${ctidy_stderr}")
string(REGEX REPLACE "[0-9]+ warnings generated\\." "" ctidy_output
                     "${ctidy_output}")
string(STRIP "${ctidy_output}" ctidy_output)
if(NOT ctidy_output STREQUAL "")
  message("${ctidy_output}")
endif()

if(NOT ctidy_result EQUAL 0)
  message(FATAL_ERROR "clang-tidy failed with exit code ${ctidy_result}")
endif()
