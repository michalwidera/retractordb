# cd build/Debug make install ; ctest -R operations-run -V
#
# Weryfikuje operator # (merge dwóch strumieni: core0 i core1).
#
# UWAGA: test używa awk zamiast compare_files z powodu race condition na
# poziomie połączenia klienta (okno między "get"+"show" a otwarciem kolejki).
#
# xretractor startuje pierwszy (lock file jako sygnał gotowości), potem xqry
# synchronicznie — unika race condition xqry-przed-xretractorem.
#
# Weryfikacja awk: - core1 (≥100) rosnące, core0 (<100) rosnące - między
# kolejnymi core1 dokładnie 2 core0 (stosunek rate 1/8 : 1/16) - sprawdzenie
# odporne na utratę pierwszych pakietów z powodu race connection.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e ; \
    rm -f *.meta *.desc str1 str2 str3 core0 core1 ; \
    xretractor query.rql -k -x & \
    while [ ! -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    xqry -s str3 -k -m 9 > out.txt ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 0.1; done ; \
    awk 'BEGIN{lc1=0;lc0=0;between=0;seen_c1=0} \
         /^[0-9]/{v=\$1+0; \
           if(v>=100){ \
             if(v<=lc1){print \"core1 not increasing at \"v; exit 1}; \
             if(seen_c1 && between!=2){print \"wrong core0 count between core1: \"between; exit 1}; \
             lc1=v; between=0; seen_c1=1 \
           } else if(v>0){ \
             if(v<=lc0){print \"core0 not increasing at \"v; exit 1}; \
             lc0=v; between++ \
           }} \
         END{if(lc1<110){print \"too few core1 values: \"lc1; exit 1}}' out.txt"
)
# set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 900)
