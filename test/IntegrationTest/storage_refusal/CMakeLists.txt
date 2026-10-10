# K2/K4 preflight przed zmiana planu na czterech drogach (#473).
get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
foreach(mode start delivery reset adhoc)
  add_test(
    NAME ${TestID}-${mode}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND bash run.sh ${mode})
  set_tests_properties(${TestID}-${mode} PROPERTIES TIMEOUT 180)
endforeach()
