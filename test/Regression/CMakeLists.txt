cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

# these set of 3 commands will copy artifacts from Regression/Data into
# build/Debug/test folder. First will cleanup this build/Debug/test/Data folder
# then copy.

file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/Data)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
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
  NAME rg-compiler-on-query
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND xretractor -q query-lnx.rql -c)

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_1
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern1
  COMMAND sh -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_2
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern2
  COMMAND sh -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_3
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern3
  COMMAND sh -c "set -e ; \
    xretractor query-unfold.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-compile_4
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern4
  COMMAND sh -c "set -e ; \
    xretractor query-crc.rql -c > out.txt ; \
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
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

add_test(
  NAME rg-compile_agse_6-vg
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern6
  COMMAND valgrind --error-exitcode=1 --leak-check=full xretractor query.rql -c)

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-xtrdb_on_text_data_2
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/Pattern7
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
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
  NAME rg-xcompiler-simple
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/simple
  COMMAND bash -c "set -e ; \
    xretractor query-simple.rql -c > out-compile.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out-compile.txt")

add_test(
  NAME rg-xcompiler-dot
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/simple
  COMMAND bash -c "set -e ; \
    xretractor query-simple.rql -c -d -f -t -s > out.dot ; \
    ${CMAKE_COMMAND} -E compare_files pattern-dot.txt out.dot")

add_test(
  NAME rg-xcompiler-simple-vg-1
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/simple
  COMMAND valgrind --error-exitcode=1 --leak-check=full xretractor
          query-simple.rql -c)

add_test(
  NAME rg-xcompiler-simple-vg-2
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/simple
  COMMAND valgrind --error-exitcode=1 --leak-check=full xretractor
          query-simple.rql -m 5)

add_test(
  NAME rg-xcompiler-simple-run
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/simple
  COMMAND
    bash -c "set -e ; \
  rm str1 str1.desc ; \
  xretractor query-simple.rql -m 10 ; \
  xtrdb < term.script > out.txt ;  \
  ${CMAKE_COMMAND} -E compare_files pattern-run.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-agse
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/agse
  COMMAND
    bash -c "set -e ; \
    xretractor query.rql -m 15 -v > verbose.txt ; \
    xtrdb < term.script > out.txt ;  \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-dsp
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/dsp
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-subquery
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/subquery
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

add_test(
  NAME rg-subquery-dot
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/subquery
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c -d -f -t -s > out.dot ; \
    ${CMAKE_COMMAND} -E compare_files pattern-dot.txt out.dot")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-dumper
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND sh -c "xretractor query-lnx.rql -c")

# -----------------------------------------------------------------------------------------------

add_test(
  NAME rg-data-consitency
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}/consistency
  COMMAND
    sh -c "set -e ; \
  xretractor -c query.rql > out_compile.txt ; \
  xretractor query.rql -m 5 -v > verbose.txt ; \
  xtrdb < term.script > out_xtrdb.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern_compile.txt out_compile.txt ; \
  ${CMAKE_COMMAND} -E compare_files pattern_xtrdb.txt out_xtrdb.txt")

# -----------------------------------------------------------------------------------------------

set_tests_properties(rg-dumper PROPERTIES DEPENDS rg-compiler-on-query)

add_test(
  NAME rg-retractor-waterfall
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND sh -c "xretractor query-lnx.rql -m 20 --waterfall")

add_test(
  NAME rg-workflow
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND test-workflow.sh query-lnx.rql)

add_test(
  NAME rg-workflow-all-operators
  WORKING_DIRECTORY ${CMAKE_REGRESSION_DATA_DIR}
  COMMAND test-all-operators.sh query-all.rql)

set_tests_properties(rg-retractor-waterfall rg-workflow
                     rg-workflow-all-operators PROPERTIES RUN_SERIAL TRUE)
