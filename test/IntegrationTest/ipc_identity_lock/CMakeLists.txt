get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xqry>)
# cmake-format: off
# Test oblal raz w trzech pelnych przebiegach `ctest -R it_ -j 4` (2026-09-22), a przechodzil
# w izolacji. Dostal wtedy RUN_SERIAL jako srodek zapobiegawczy. Tekst tamtej awarii przepadl,
# bo `ctest --rerun-failed` NADPISUJE Testing/Temporary/LastTest.log.
#
# PRZYCZYNA POZOSTAJE NIEUSTALONA. Ponizej stan po polowaniu z 2026-09-22.
#
# Dlaczego RUN_SERIAL go NIE ma. Decyzja wynika z werdyktu, a nie z tego, ze przestalo oblewac:
# przy nieznanej przyczynie RUN_SERIAL nie leczy niczego, za to ODBIERA jedyne warunki, w ktorych
# zjawisko bylo widziane - a verify.py umie juz o sobie opowiedziec, wiec nastepne wystapienie
# przyniesie komplet dowodow zamiast jednego zdania. Zeby wrocic do srodka zapobiegawczego,
# wystarczy dopisac RUN_SERIAL TRUE do wiersza na koncu tego pliku.
#
# Polowanie: 70 przebiegow `ctest -R it_ -j 4` BEZ RUN_SERIAL, log kopiowany po kazdym
# przebiegu. 70/70 zielonych, zero oblan. Przy zdarzeniu rzadkim cisza znaczy tylko cisze:
# 70 czystych przebiegow daje gorne ograniczenie czestosci okolo 4% (regula trzech, 95%), wiec
# odczyt "jedna awaria na trzy przebiegi" odpada, a "zdarzenie rzadkie" - nie.
#
# Dodatkowo 12 pelnych przebiegow zestawu w kontenerze CI (`scripts/test-ci.sh --profile commit`,
# 4 vCPU, 8 GiB, /dev/shm 64 MiB), srednio 350 s - 12/12 zielonych. To inne warunki niz host:
# na 24 bezczynnych rdzeniach `-j 4` niczego nie dociska, a w kontenerze te same cztery zadania
# nasycaja maszyne. Przepelnienie /dev/shm, ktore jest fail-open (ClaimStatus::Unavailable po
# cichu zdejmuje izolacje przestrzeni nazw), nie wystapilo ani razu.
#
# Co zostalo wykluczone i czym:
#   * glodzenie CPU - 50 przebiegow samego verify.py, polowa przy nasyceniu 24 rdzeni, 0 oblanych
#     (verify.py ma twarde limity 10 s / 20 s na podprocesach);
#   * timeout klienta jako powod "pierwszy serwer przestal odpowiadac" - budzet to
#     ipcClientResponseMaxFails * kClientResponsePollInterval = 300 * 10 ms = 3 s (appConfig.hpp,
#     constants.hpp), a nie 100 ms; zaglodzenie watku komunikacyjnego na 3 s pod `-j 4` na
#     24 rdzeniach nie jest wiarygodne;
#   * odtworzenie obiektow przez klienta - kazda sciezka xqry otwiera je przez `open_only`
#     (ipcClient.cpp:111,112,117; qryLauncher.cpp:65,66), wiec klient nie moze zalozyc obiektu
#     globalnego na nowo i zmienic i-wezla;
#   * segment magistrali w porownywanym zbiorze - nazywa sie `xrdbbus_v6_<przestrzen>`
#     z PODKRESLENIEM, a wzorzec verify.py wymaga kropki;
#   * sasiad z tego samego slotu puli (it12: issue167_dedup_positive, window_aggregate,
#     self_ref_simplify_type, lang_showcase, agse_array) - sprawdzone ponownie: kazdy wola
#     xretractora synchronicznie w lancuchu `&&`, zaden nie startuje serwera w tle;
#   * zabranie obiektow ZYWEJ instancji przez zamiatacz pozostalosci - zmierzone wprost:
#     odpowiednik claimAbandoned() hamerowal blokade tozsamosci zywego serwera 6 044 611 razy
#     w 20 s, udanych zajec 0, komplet obiektow ani razu nie byl niepelny.
#
# ZNALEZIONY I NAPRAWIONY MECHANIZM (realny, ale NIE jest to dowiedziona przyczyna tamtej awarii).
# verify.py zajmowal blokade tozsamosci przez `open()` + `flock()`, czyli dwa wywolania jadra bez
# zabezpieczenia, podczas gdy silnik domyka to okno sprawdzeniem stillLinked i petla ponowien
# (lockFile.cpp). Trafiony w to okno zamiatacz sasiada zajmuje i-wezel i go ODLINKOWUJE, a wtedy
# test trzyma uchwyt do pliku, ktorego pod ta sciezka juz nie ma.
#
# Obie galezie wyscigu zaobserwowane na stanowisku obciazeniowym (64 testy moglace biec obok
# celu + cztery petle `xretractor --cleanup`, razem okolo 500 zamiatan/s zamiast 4,3/s
# z normalnego przebiegu):
#   * 33 razy zamiatacz trzymal blokade w chwili flock celu -> BlockingIOError,
#   *  8 razy sciezka byla juz wolna -> "odrzucany" xretractor zakladal nowy plik i STARTOWAL
#      zamiast odmowic (kontrola krok1:odmowa-kod-wyjscia).
#
# Liczby, ktore z tego wynikaja:
#   * przed poprawka, gestosc x116:    41 / 697 iteracji = 5,9%
#   * po poprawce, ta sama gestosc:     0 / 2705 iteracji (gorne ograniczenie 95%: 0,11%)
#     - przy starej czestosci awarii byloby tam okolo 159;
#   * przed poprawka, gestosc realna:   0 / 1150 iteracji - stad wniosek, ze samo stanowisko
#     bez wzmocnienia NIE MA czulosci na wyscig o oknie mikrosekundowym;
#   * ekstrapolacja na gestosc realna:  okolo 4e-4 na przebieg, czyli raz na 2-3 tysiace.
# Ostatnia liczba jest o trzy rzedy wielkosci mniejsza niz obserwacja "raz na trzy przebiegi",
# wiec ten mechanizm tamtej awarii NIE tlumaczy - choc byl prawdziwa droga, ktora sasiad
# potrafil ten test wywrocic, i ta droga jest juz zamknieta.
#
# Sprostowanie do wczesniejszego zapisu: globalne zamiatanie /tmp to NIE tylko
# `xretractor --cleanup`. sweepAbandonedResources wykonuje KAZDY konczacy sie serwer
# (executorsm.cpp: cleanup, uzbrajane w executorsm::run), wiec w przebiegu rownoleglym dzieje sie
# to setki razy, obok tego testu. Wykluczenie "it_leftover_sweep jest RUN_SERIAL, wiec obok biec
# nie moze" obejmowalo tylko jednego z wielu wolajacych. Straz sama w sobie jest szczelna (wyzej),
# ale okno w tescie zamykalo sie wlasnie o ten mechanizm.
#
# Gdy test obleje ponownie: verify.py zrzuca na stdout - W CHWILI AWARII, przed sprzataniem -
# pelne /dev/shm z i-wezlami, blokady z /tmp z wlascicielami z /proc/locks, tablice procesow,
# stan i log serwera, ogon logu silnika z TMPDIR oraz os czasu obserwatora (/dev/shm co 2 ms).
# Log trzeba skopiowac, ZANIM cokolwiek sie powtorzy; `scripts/collect-test-failures.py
# <build-dir> --ctest-status N` zbiera komplet dowodow.
#
# TIMEOUT 120, a nie 60: wlasne limity verify.py sumuja sie w najgorszym razie do 60 s
# (20 s na `--hello -w` plus cztery po 10 s), wiec przy progu 60 s CTest zabilby test ZANIM
# zdazylby wypisac diagnoze - czyli dokladnie w przypadku, dla ktorego ta diagnoza powstala.
# cmake-format: on
set_tests_properties(${TestID} PROPERTIES TIMEOUT 120)
