cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(regression-test)

set(CMAKE_REGRESSION_DATA_DIR ${CMAKE_SOURCE_DIR}/test/Regression/Data)

# Purpose of this test is to fail if someone will create xrdb artifact If such
# file will appear in .local/bin folder or in a path this will cause to system
# hang during startup My fail count #2

add_test(
  NAME check-output-artifacts
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND test-artifacts.sh xrdb)

# -----------------------------------------------------------------------------------------------

add_test(
  NAME compiler-test-on-query
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND xcompiler -q query-lnx.rql)

# -----------------------------------------------------------------------------------------------

add_test(
  NAME xcompile_source-1
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern1
  COMMAND sh -c "xcompiler -q query.rql > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME xcompile_source-2
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern2
  COMMAND sh -c "xcompiler query.rql > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt
  ")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME xcompile_source-3
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern3
  COMMAND sh -c "xcompiler query-unfold.rql > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME xcompile_source-4
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern4
  COMMAND sh -c "xcompiler query-crc.rql > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt
  ")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME xtrdb_on_script-5
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern5
  COMMAND sh -c "xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME xcompile_agse-6
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern6
  COMMAND bash -c "xcompiler query.rql > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME xtrdb_on_text_data_src-8
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern8
  COMMAND bash -c "xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")
# -----------------------------------------------------------------------------------------------

add_test(
  NAME xtrdb_on_bin_data_src-9
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern9
  COMMAND bash -c "xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------
add_test(
  NAME dumper-test
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND sh -c "xcompiler -q query-lnx.rql; xdumper -d")

set_tests_properties(dumper-test PROPERTIES DEPENDS compiler-test-on-query)

add_test(
  NAME retractor-waterfall
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND sh -c "xcompiler query-lnx.rql; xretractor -m 20 --waterfall")

add_test(
  NAME workflow-test
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND test-workflow.sh query-lnx.rql)

add_test(
  NAME workflow-all-operators
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND test-all-operators.sh query-all.rql)

set_tests_properties(retractor-waterfall workflow-test workflow-all-operators
                     PROPERTIES RUN_SERIAL TRUE)
