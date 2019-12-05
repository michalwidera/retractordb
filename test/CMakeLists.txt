cmake_minimum_required(VERSION 2.8 FATAL_ERROR )

project( database-test )

enable_testing()

add_test( NAME compier-selftest
          WORKING_DIRECTORY .
          COMMAND build/xcompiler -t
         )

add_test( NAME compier-test-on-query
          WORKING_DIRECTORY .
          COMMAND build/xcompiler -q test/query-lnx.txt 
         )

add_test( NAME compier-test-on-dumper 
          WORKING_DIRECTORY .
          COMMAND sh -c "build/xdumper > test/out.txt"
         )

add_test(
    NAME compare_outputs 
    WORKING_DIRECTORY . 
    COMMAND ${CMAKE_COMMAND} -E compare_files test/pattern.txt test/out.txt
)

add_test( NAME dumper-test
          WORKING_DIRECTORY .
          COMMAND build/xdumper -d
         )

add_test( NAME executor-waterfall
          WORKING_DIRECTORY .
          COMMAND build/xabracadabra -m 20 --waterfall 
         )

add_test( NAME workflow-test
          WORKING_DIRECTORY .
          COMMAND test/test-workflow.sh > test/out.workflow.txt
         )

add_test( NAME workflow-all-operators
          WORKING_DIRECTORY .
          COMMAND test/test-all-operators.sh 
         )




set(SOURCES_TEST
  regtest/query.txt
  regtest/gotest.sh
  regtest/query-lnx.txt
)
