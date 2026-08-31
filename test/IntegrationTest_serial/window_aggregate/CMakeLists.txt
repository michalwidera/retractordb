# Agregat okna rekordowego w liscie SELECT: AGG(pole : szerokosc).
#
# Szerokosc liczy REKORDY, wiec pole tablicowe redukuje szerokosc*N wartosci.
# Wzorzec jest policzony recznie: porownanie z inna konstrukcja silnika
# sprawdzaloby zgodnosc dwoch implementacji, a nie zgodnosc z definicja. Jedyny
# wyjatek jest tozsamoscia — MIN(a:1) kontra FROM MIN(src) — i wlasnie dlatego
# nadaje sie na kotwice.

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
    ${CMAKE_BINARY_DIR}/src/retractor/xretractor query.rql -m 8 -f > verbose.txt && \
    cd temp && ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < ../term.script > ../out.txt && cd .. && \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
