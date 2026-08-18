# K24/H10: ogon `-`, `Theta` i `~Theta` liczony postacia DOKLADNA (2026-08-18).
#
# Bramka od strony wyjscia, nie rachunku. Ogon jest obserwowalny jako liczba
# rekordow — strumien milczy przez pierwsze `tail` slotow — wiec powrot dawnej
# reguly (zawyzajacej o slot) zmieni `size` we wzorcu i test oblewa.
#
# Wartosci wzorca sa zgodne z modelem zdarzeniowym kampanii K24e
# (rdb-experiment/results_20260818_K24e), a nie odczytane z silnika w ciemno:
# th_int 0, th_fr 1, sub_over 0, th_deep 1, nth_deep 0.
#
# Dwa przypadki NIE zmieniaja sie wzgledem stanu sprzed naprawy (th_fr, th_deep)
# i sa w tym tescie celowo: bez nich bramka przechodzilaby takze dla reguly,
# ktora zeruje ogon rozplotu zawsze.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e && \
    rm -rf temp && mkdir -p temp && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 60 > verbose.txt && \
    cd temp && \
    ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < ../term.script > ../out.txt && \
    cd .. && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
