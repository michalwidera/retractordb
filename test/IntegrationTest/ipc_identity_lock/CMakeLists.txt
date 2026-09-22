get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xqry>)
# cmake-format: off
# RUN_SERIAL jako srodek zapobiegawczy, nie jako naprawa znanej przyczyny. Test
# oblal raz w trzech pelnych przebiegach `ctest -R it_ -j 4` (2026-09-22), a
# przechodzil w izolacji. Tekst tamtej awarii przepadl, bo `ctest --rerun-failed`
# NADPISUJE Testing/Temporary/LastTest.log - przy nastepnym oblaniu log trzeba
# skopiowac, ZANIM cokolwiek sie powtorzy.
#
# Co zostalo wykluczone dowodem, zeby nie szukac tam drugi raz:
#   * glodzenie CPU (verify.py ma twarde limity 10 s / 20 s na podprocesach):
#     50 przebiegow verify.py, polowa przy nasyceniu 24 rdzeni, 0 oblanych;
#   * zamiatanie `xretractor --cleanup` z it_leftover_sweep, ktore skanuje /tmp
#     GLOBALNIE i kasuje dokladnie te trzy obiekty - tamten test jest juz
#     RUN_SERIAL, wiec obok biec nie moze;
#   * segment magistrali w zbiorze `before` - nazywa sie `xrdbbus_v6_<przestrzen>`
#     z PODKRESLENIEM, a wzorzec verify.py wymaga kropki, wiec nie wpada;
#   * wyscig open()/flock() na blokadzie tozsamosci - lockFile.cpp sprawdza
#     stillLinked() po zalozeniu blokady i ponawia, a claimAbandoned() pomija
#     plik, ktorego blokade ktos trzyma.
#
# Hipoteza o sasiedzie z tego samego slotu puli (it12): taki sasiad startuje
# serwer BEZ `--name`, wiec bierze nazwe instancji z RDB_NAMESPACE - czyli te
# nazwy obiektow IPC, ktore verify.py porownuje po i-wezlach. RESOURCE_LOCK
# rozdziela takie testy w czasie, ale nie objalby procesu, ktory przezyl swoj
# test. Sprawdzone: ZADEN z sasiadow it12 nie uruchamia serwera w tle - wszystkie
# wolaja go synchronicznie w lancuchu `&&` z ograniczonym `-m N`, wiec powloka
# czeka na pelne wyjscie razem ze sprzataniem IPC. Hipoteza jest wiec slaba, choc
# nie da sie jej wykluczyc calkiem: bramki higieny (serverlib.sh) nie ma tu zaden
# katalog.
#
# Stan na 2026-09-22: PRZYCZYNA NIEZNANA. Awarii nie udalo sie odtworzyc ani
# w 50 przebiegach samego verify.py (polowa przy nasyceniu 24 rdzeni), ani
# w 8 czystych przebiegach calego zestawu `-j 4` po dodaniu RUN_SERIAL. Osiem
# czystych przebiegow jest zgodne z tym, ze RUN_SERIAL pomogl, ale rownie dobrze
# z tym, ze rzadkie migotanie po prostu nie wrocilo - z JEDNEJ zaobserwowanej
# awarii nie da sie oszacowac czestosci, wiec porownanie "przed/po" nic tu nie
# rozstrzyga. Nie traktowac tego wpisu jako zamknietej diagnozy.
# cmake-format: on
set_tests_properties(${TestID} PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
