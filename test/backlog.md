# Backlog silnika

Lista zmian w silniku (`src/`), które trzeba wdrożyć w przyszłości, ale które świadomie nie weszły do sesji, w której się pojawiły. Źródłem wpisu jest zwykle praca nad czymś innym: przy naprawie testu, przeglądzie kodu albo przebiegu ablacji wychodzi pomysł, brakująca reguła lub brakujące sprawdzenie. Zgodnie z `CLAUDE.md` („Surgical edits") taka rzecz nie jest robiona przy okazji - zostaje zapisana tutaj, a decyzja o realizacji należy do człowieka i jest osobnym zadaniem.

Plik jest pamięcią projektu, nie trackerem zgłoszeń. Wpis ma pozwolić podjąć temat po tygodniach bez odtwarzania kontekstu: skąd się wziął, jak wygląda kod dziś, jak go zrealizować i po czym poznać, że jest gotowe.

## Co tu trafia

- Reguły i przebiegi kompilatora, zmiany ewaluatora, runtime'u i klientów (`xqry`, `xtrdb`), odłożone świadomie.
- Brakujące sprawdzenia, które zamieniłyby awarię w wykonaniu na błąd kompilacji.

Nie trafiają tu: defekty psujące wynik lub wywracające proces - te zgłasza się od razu człowiekowi; zmiany dokumentacji; stan kampanii badawczych.

## Format wpisu

Indeksy `B-NNN` są kolejne i nigdy nie są używane ponownie. Wpisu się nie usuwa: po realizacji albo odrzuceniu zmienia się tylko jego status, dopisując commit albo powód. Nowe wpisy dopisuje się na końcu.

```markdown
## B-NNN - tytuł

- **Data:** RRRR-MM-DD
- **Status:** pomysł | przyjęte | w realizacji | zrobione (commit) | odrzucone (powód)
- **Źródło:** skąd wpis się wziął - sesja, test, przebieg
- **Stan obecny:** jak to wygląda w kodzie dziś, z plikami i funkcjami
- **Pomysł na realizację:** konkretne kroki
- **Do rozstrzygnięcia:** ryzyka, skutki uboczne, alternatywy
- **Gotowe gdy:** kryteria sprawdzalne testem
```

---

## B-001 - Reguła C (`x+0 -> x`) pod przełącznikiem `aggressive_expr_optimization`

- **Data:** 2026-09-18
- **Status:** pomysł - do decyzji
- **Źródło:** `it_dot_labels-compile` i `it_dot_labels-dot` oblewały build ablacyjny all-off. Szablon `core[0]+$` dawał dla `$`=0 wyrażenie `core[0]+0`, które znika z planu tylko przy `RDB_OPT_SIMPLIFY_EXPRESSIONS=ON`, więc listing zależał od przełącznika wydajnościowego. Test naprawiono po stronie testu (`SELECT w[$] STREAM ch[2] FROM w`), więc ten wpis nie jest potrzebny, żeby test był zielony. Zostaje jako propozycja zmiany w optymalizatorze.
- **Stan obecny:** reguła C już istnieje. `dropNeutralOperand()` w `src/retractor/lib/exprSimplify.cpp` usuwa `E+0`, `E-0`, `E*1` i `E/1` wyłącznie dla typów o arytmetyce dokładnej i tylko wtedy, gdy stała ma tę samą reprezentację co podwyrażenie (inaczej skasowałaby promocję typu, np. `bajt + 0`). Działa razem z regułami A (zwijanie stałych) i B (reasocjacja ogona stałych) w `simplifyFieldExpressions()`, wołanym w `compiler.cpp` pod `#if RDB_OPT_SIMPLIFY_EXPRESSIONS`, czyli za przełącznikiem ablacyjnym. Za `aggressive_expr_optimization` (domyślnie OFF) stoi dziś wyłącznie reguła D (`a*a -> a^2`). Testy reguły C: `test/UnitTest/test_exprSimplify.cpp` - `drops_neutral_operands`, `drops_neutral_operand_written_on_the_left`, `keeps_neutral_operand_of_a_wider_type`.
- **Pomysł na realizację:**
  1. W `exprSimplify.cpp` objąć jedyne wywołanie `dropNeutralOperand()` (koniec funkcji upraszczającej ogon stałych) warunkiem `#if aggressive_expr_optimization`, tak jak regułę D.
  2. Zaktualizować opis reguł w `exprSimplify.hpp` oraz komentarz i opis `option(aggressive_expr_optimization ...)` w głównym `CMakeLists.txt` (dziś „rule D").
  3. Testy reguły C w `test_exprSimplify.cpp` objąć tym samym warunkiem, a dla domyślnego buildu dodać asercję, że `E+0` zostaje w planie.
  4. Przejrzeć wzorce testów integracyjnych, w których element neutralny znika dziś z listingu.
- **Do rozstrzygnięcia:**
  - Zmienia plany domyślnego buildu: każde `E+0` i `E*1` zostaje w planie i jest liczone w każdym slocie. To regres wydajności domyślnej konfiguracji w zamian za niezależność listingu.
  - Zmienia znaczenie przełącznika: `aggressive_expr_optimization` jest dziś zarezerwowany dla przepisań, które ruszają korpus H9 (`validate_corpus.py::require_main_r3_zero`). Reguła C jest zwykłym uproszczeniem wydajnościowym i pasuje do `RDB_OPT_SIMPLIFY_EXPRESSIONS`.
  - Nie usuwa zależności listingu od konfiguracji, tylko przenosi ją na przełącznik, którego macierz ablacji w CI nie sprawdza.
  - Alternatywa bez zmiany silnika: zasada dla testów - test, który nie dotyczy optymalizatora, nie zawiera wyrażeń, które optymalizator może przepisać (tak naprawiono `it_dot_labels`).
- **Gotowe gdy:** przy domyślnej konfiguracji reguła C nie działa, a przy `aggressive_expr_optimization=ON` działa; `ctest` zielony w Debug i Release; `ninja test_gate` zielona; pełna seria w buildzie all-off zielona (zmiana dotyka optymalizatora).

## B-002 - `--autoname` ponawia losowanie, gdy nazwa jest zajęta

- **Data:** 2026-09-18
- **Status:** pomysł
- **Źródło:** plan wieloserwerowości, etap 2e („`--autoname` nadal losuje raz, bez ponowienia przy trafieniu w nazwę żywej instancji"); plan przeniesiony do `paper-arXiv/debs/done/multiserver_plan.md`.
- **Stan obecny:** `src/retractor/launcher.cpp:314-316` woła `servername::generate()` raz; nazwa to jedna z 4096 kombinacji (64 przymiotniki × 64 nazwiska w `src/retractor/lib/serverName.cpp`) i nikt nie sprawdza, czy jest wolna. Trafienie w nazwę żywej instancji nie robi szkody - blokada instancji jest brana przed kasowaniem artefaktów (etap 2f) - ale start kończy się odmową i operator musi uruchomić program ponownie.
- **Pomysł na realizację:** losować w pętli (np. do 16 prób) i dla każdej nazwy od razu próbować przejąć jej blokadę - `flock` jest atomowym testem wolności, więc nie ma wyścigu między sprawdzeniem a zajęciem. Nazwę wypisywać na stdout (`Instance name: ...`) dopiero po udanym przejęciu.
- **Do rozstrzygnięcia:** nazwa powstaje dziś przed kompilacją, a blokada jest brana po niej, więc pętla musi objąć moment przejęcia blokady albo losowanie trzeba tam przenieść. Test deterministyczny wymaga szwu w generatorze (ziarno albo hak `RDB_FAULT_*`), bo trafienia 1/4096 nie da się wymusić losowaniem.
- **Gotowe gdy:** test integracyjny z nazwą zajętą przez żywą instancję (wymuszoną hakiem) pokazuje start pod inną nazwą, a bez kolizji zachowanie się nie zmienia; `ctest` zielony w Debug i Release; `ninja test_gate` zielona.

## B-003 - Testy jednostkowe na bezimiennych obiektach IPC bez `RUN_SERIAL`

- **Data:** 2026-09-18
- **Status:** pomysł
- **Źródło:** plan wieloserwerowości, etap 2i („Czego 2i nie objęło"); plan w `paper-arXiv/debs/done/multiserver_plan.md`.
- **Stan obecny:** `test/UnitTest/CMakeLists.txt` (blok `set_tests_properties ... RUN_SERIAL TRUE`) trzyma szeregowo `ut_rdb`, `ut_xqry`, `ut_dataModel` (z `-compile` i `-compare`), `ut_soperations`, `ut_ipcServer` i `ut_ipcClient`. Nazwa instancji IPC jest wpisana w kodzie C++ tych testów, więc `RDB_NAMESPACE` z etapu 2i do nich nie sięga. `ut_ipcClient` dopisano 2026-09-14 po `boost::interprocess_exception::library_error` przy `ctest -j 8`; przyczyny nie zbadano. Według pomiaru z etapu 2i te testy są częścią ~33 s szeregowego ogona Debug.
- **Pomysł na realizację:** testy biorą nazwę instancji z `servername::environmentNamespace()` (z własną nazwą zastępczą, gdy zmienna jest pusta), a `test/UnitTest/CMakeLists.txt` przydziela im przestrzeń z tej samej puli 16 co testom integracyjnym (`RDB_NAMESPACE` + `TMPDIR` + `RESOURCE_LOCK`). Potem zdjąć `RUN_SERIAL`.
- **Do rozstrzygnięcia:** `ut_ipcServer` kasuje globalne nazwy IPC - po parametryzacji ma kasować wyłącznie własne. Najpierw ustalić przyczynę `library_error` w `ut_ipcClient`, bo samo przeniesienie do przestrzeni nazw może ją tylko zamaskować.
- **Gotowe gdy:** blok `RUN_SERIAL` zniknął; `ctest -j 24` zielony w kilku kolejnych przebiegach Debug i Release; po przebiegu w `/dev/shm` nie zostają obiekty testów.

## B-004 - Rotacja logu `xretractor`

- **Data:** 2026-09-18
- **Status:** pomysł
- **Źródło:** plan wieloserwerowości, etap 2i („Plik logu ścieżki bezimiennej nadal rośnie w `TMPDIR` bez rotacji", 94 MB w pomiarze z 2026-09-03); plan w `paper-arXiv/debs/done/multiserver_plan.md`.
- **Stan obecny:** `src/common/uxSysTermTools.cpp:174` tworzy `basic_file_sink_mt` na `<TMPDIR>/<nazwa programu>.log` w trybie dopisywania (`truncate = false`), bez limitu rozmiaru i bez rotacji. Nazwa pliku nie zależy od nazwy instancji, więc wszystkie instancje w jednym `TMPDIR` piszą do jednego pliku; osobny `TMPDIR` mają tylko testy integracyjne z przestrzenią nazw. `daily_file_sink.h` jest dołączony w tym pliku, ale nieużywany.
- **Pomysł na realizację:** zastąpić sink `rotating_file_sink_mt` (np. 10 MB × 3 pliki), z limitami w TOML (`[log] max_size_mb`, `max_files`) i dotychczasowym zachowaniem jako wartością domyślną tylko wtedy, gdy klucza nie ma.
- **Do rozstrzygnięcia:** rotacja spdlog nie jest bezpieczna między procesami - kilka instancji piszących do jednego pliku wymaga nazwy instancji w nazwie pliku albo zgody na utratę części wpisów przy rotacji. `scripts/collect-test-failures.py` zbiera `xretractor.log` z `TMPDIR` - musi zbierać także pliki po rotacji i ewentualną nową nazwę.
- **Gotowe gdy:** log nie przekracza limitu w długim przebiegu; `collect-test-failures.py` nadal dołącza logi do raportu; `ctest` zielony w Debug i Release.

## B-005 - Automatyczna naprawa segmentu magistrali po śmierci twórcy

- **Data:** 2026-09-18
- **Status:** pomysł - do decyzji
- **Źródło:** plan wieloserwerowości, etap 2b („Czego 2b nie objęło"); plan w `paper-arXiv/debs/done/multiserver_plan.md`.
- **Stan obecny:** `src/retractor/lib/bus.cpp` - twórca segmentu publikuje `magic` na końcu inicjalizacji, a podłączający się czeka `kInitWaitLimit` (2 s). Gdy `magic` się nie pojawi, instancja startuje bez magistrali (fail-open: rozłączność nazw nie jest egzekwowana), a log mówi „remove /dev/shm/<segment> to repair". Twórca, który sam wykryje porażkę przed publikacją `magic`, kasuje swój segment (komentarz przy tej ścieżce w `bus.cpp`), ale twórca zabity w oknie między `create_only` a zapisem `magic` zostawia segment martwy do ręcznego usunięcia. Okno trwa mikrosekundy.
- **Pomysł na realizację:** plik blokady obok segmentu (np. w katalogu blokad) trzymany `flock`-iem przez całą inicjalizację. Czekający po upływie limitu próbuje wziąć ten `flock`; jeżeli go dostaje, a `magic` nadal nie ma, twórca na pewno nie żyje (jądro zwalnia `flock` przy śmierci procesu), więc można pod tą samą blokadą skasować segment i założyć go od nowa.
- **Do rozstrzygnięcia:** to jest „drugi protokół wzajemnego wykluczania nad tym samym segmentem", który plan świadomie odrzucił. Kasowanie przy żywych odwzorowaniach prowadzi do dwóch segmentów i cichej utraty rozłączności (etap 2e, „Wersja w nazwie segmentu"), więc blokada musi obejmować każde podłączenie, nie tylko inicjalizację. Koszt: dodatkowy plik i `flock` przy każdym starcie oraz przy każdym odczycie magistrali przez `xqry`.
- **Gotowe gdy:** test z hakiem zabijającym twórcę między `create_only` a zapisem `magic` pokazuje, że kolejna instancja sama odtwarza segment i egzekwuje rozłączność; `test_bus` zielony pod valgrindem; `it_multiserver_*` zielone.

## B-006 - Budżet `/dev/shm` na subskrybenta konfigurowany per instancja

- **Data:** 2026-09-18
- **Status:** zrobione przed założeniem wpisu (`81d03e48`, `300f23c6`) - wpis zostaje jako ślad sprawdzenia
- **Źródło:** plan wieloserwerowości, „Rzeczy, których magistrala świadomie nie obejmuje": subskrypcja strumienia zajmuje `(1/interval) × 10 s × 1024 B`, przy 720 Hz ~7,4 MB na subskrybenta, co na workerze Pi 400 jest realnym ograniczeniem; plan proponował rozważyć `ipcQueueBufferSeconds` per instancja.
- **Stan obecny:** klucz TOML `ipc.queue_buffer_seconds` (domyślnie 10 s, `src/retractor/lib/appConfig.hpp`, walidacja w `appConfig.cpp`) istnieje od PR #182 (`81d03e48`, 2026-06-21). Każda instancja może dostać własny plik przez `--config <plik>`, więc ogranicznik per instancja jest dostępny. `xretractor <plan> -c --shmbudget` (od PR #239, `300f23c6`) wypisuje budżet pamięci dzielonej planu (`shmbudget::report`).
- **Pomysł na realizację:** brak - nic do zrobienia w silniku.
- **Do rozstrzygnięcia:** czy potrzebny jest limit per subskrypcja (per klient `xqry`), a nie per instancja - dziś żaden scenariusz tego nie wymaga.
- **Gotowe gdy:** spełnione.
