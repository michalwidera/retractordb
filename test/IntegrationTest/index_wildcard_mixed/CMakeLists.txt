# Dziki indeks `[_]` obok innych pozycji listy SELECT.
#
# Rozwiniecie zmienia UKLAD PAYLOADU, a nie tylko liczbe wpisow deskryptora,
# wiec kontrola musi siegnac po wartosci. Wzorzec jest policzony recznie z a.txt
# i b.txt: porownanie z innym zapisem tego samego silnika sprawdzaloby zgodnosc
# dwoch implementacji, a nie zgodnosc z definicja.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c
    "set -e && \
    rm -rf ./temp && mkdir -p ./temp && \
    rm -f ./*.desc ./*.meta ./*.shadow out.txt && \
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 6 -f > verbose.txt && \
    cd temp && ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < ../term.script > ../out.txt && cd .. && \
    bash ../compare.sh --ignore-eol pattern.txt out.txt")
