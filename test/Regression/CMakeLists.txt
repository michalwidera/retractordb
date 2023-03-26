cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(regression-test)

# these set of 3 commands will copy artifacts from Regression/Data into
# build/Debug/test folder. First will cleanup this build/Debug/test/Data folder
# then copy.

file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/Data)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_REGRESSION_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data)

# Purpose of this test is to fail if someone will create xrdb artifact If such
# file will appear in .local/bin folder or in a path this will cause to system
# hang during startup My fail count #2

add_test(
  NAME rg-check-banned-artifacts
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND fail-if-file-exist.sh xrdb)

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compiler-test-on-query
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND xretractor -q query-lnx.rql -c)

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_1
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern1
  COMMAND sh -c "set -e ; \
  xretractor query.rql -c > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_2
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern2
  COMMAND sh -c "set -e ; \
    xretractor query.rql -c > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_3
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern3
  COMMAND sh -c "set -e ; \
  xretractor query-unfold.rql -c > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_4
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern4
  COMMAND sh -c "set -e ; \
    xretractor query-crc.rql -c > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-xtrdb_5
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern5
  COMMAND sh -c "set -e ; \
  xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_agse_6
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern6
  COMMAND bash -c "set -e ; \
  xretractor query.rql -c > out.txt ; xdumper >> out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-xtrdb_on_text_data
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern8
  COMMAND bash -c "set -e ; \
  xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")
# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-xtrdb_on_bin_data
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern9
  COMMAND bash -c "set -e ; \
  xtrdb < term.script > out.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------
add_test(
  NAME rg-dumper-test
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND sh -c "xretractor query-lnx.rql -c; xdumper -d")

set_tests_properties(rg-dumper-test PROPERTIES DEPENDS
                                               rg-compiler-test-on-query)

add_test(
  NAME rg-retractor-waterfall
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND sh -c "xretractor query-lnx.rql -m 20 --waterfall")

add_test(
  NAME rg-workflow-test
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND test-workflow.sh query-lnx.rql)

add_test(
  NAME rg-workflow-all-operators
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND test-all-operators.sh query-all.rql)

set_tests_properties(rg-retractor-waterfall rg-workflow-test
                     rg-workflow-all-operators PROPERTIES RUN_SERIAL TRUE)
