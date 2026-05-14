# cd build/Debug make install ; ctest -R operations-run -V
#
# Weryfikuje operator # (merge dwóch strumieni: core0 i core1).
#
# UWAGA: test używa grep zamiast compare_files z powodu nieusuwalnego race condition
# na poziomie połączenia klienta. Sekwencja:
#
#   xretractor tworzy IPC → xqry wykrywa IPC (~100ms polling) →
#   xqry wysyła "get"+"show" (2 round-tripy IPC) → xqry otwiera producer queue
#
# W tym oknie (~200ms, wydłużonym przez valgrind) core0 zdąży wyprodukować
# pierwsze 1-2 pakiety (rate 1/16). Pakiety te są bezpowrotnie utracone —
# strumienie nie buforują danych dla późno podłączonych klientów.
#
# Dodanie ONESHOT do core0 nie rozwiązuje problemu: rate produkcji jest taki sam,
# więc okno połączenia traci te same pierwsze pakiety.
#
# Właściwe rozwiązanie wymagałoby replay po stronie serwera (xretractor).
# Do tego czasu weryfikujemy kolejność wartości z obu strumieni za pomocą awk:
# - core1 (≥100) rosnące, core0 (<100) rosnące
# - między kolejnymi core1 dokładnie 2 core0 (stosunek rate 1/8 : 1/16)
# - sprawdzenie odporne na utratę pierwszych pakietów z powodu race connection.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    rm -f *.meta *.desc str1 str2 str3 core0 core1 ; \
    xqry --wait-server -s str3 -k -m 9 > out.txt & \
    XQRY_PID=$! ; \
    xretractor query.rql -k -x & \
    wait \$XQRY_PID ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
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
         END{if(lc1<110){print \"too few core1 values: \"lc1; exit 1}}' out.txt")
# set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 900)
