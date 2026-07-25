Pracujemy w repozytorium:

/home/michal/github/retractordb

Cel: przygotować RetractorDB do kontrolowanych badań ablacyjnych optymalizatora planu zapytań.

Gotowe, gdy:
1. każdą badaną optymalizację można niezależnie włączyć lub wyłączyć podczas konfiguracji CMake;
2. domyślna konfiguracja zachowuje obecne działanie systemu;
3. wyłączenie optymalizacji nie wyłącza etapów wymaganych do utworzenia poprawnego planu;
4. można jednoznacznie ustalić, z jakimi przełącznikami została zbudowana binarka;
5. konfiguracja domyślna i konfiguracje z wyłączonymi optymalizacjami kompilują się oraz przechodzą odpowiednie testy semantyczne;
6. nie są jeszcze wykonywane benchmarki ani właściwa analiza ablacyjna.

Najpierw:
- użyj umiejętności `retractordb-system`;
- przeczytaj `CLAUDE.md`;
- uruchom kontrolę aktualności wiedzy projektu;
- sprawdź `git status` i zachowaj wszystkie istniejące zmiany użytkownika;
- przejrzyj aktualną implementację `compiler::compile()`, `deduplicateSubstrats()`, `shareEquivalentSelectComputations()` i `factorMatchedHashTimeMoves()`;
- przedstaw plan zmian i listę plików, a przed implementacją poczekaj na moje zatwierdzenie zgodnie z zasadami repozytorium.

Po zatwierdzeniu zaimplementuj poniższe wymagania.

1. Dodaj cztery opcje CMake

Wszystkie opcje mają być domyślnie włączone, aby zwykły build zachowywał obecne działanie:

    RDB_OPT_DEDUP_SUBSTRATES=ON
    RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON
    RDB_OPT_COMMUTATIVE_ADD=ON
    RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=ON

Znaczenie opcji:

A. RDB_OPT_DEDUP_SUBSTRATES

Steruje wywołaniem:

    compiler::deduplicateSubstrats()

Po ustawieniu `OFF` kompilator nie może scalać identycznych substratów ani przepisywać ich referencji na jeden wspólny strumień.

B. RDB_OPT_SHARE_EQUIVALENT_SELECTS

Steruje wywołaniem:

    compiler::shareEquivalentSelectComputations()

Po ustawieniu `OFF` kompilator nie może tworzyć wspólnego substratu dla równoważnych publicznych zapytań `SELECT`. Każdy publiczny `SELECT` ma zachować własne obliczenie.

C. RDB_OPT_COMMUTATIVE_ADD

Steruje wyłącznie uwzględnianiem prawa:

    A + B ≡ B + A

podczas tworzenia podpisów planów w `shareEquivalentSelectComputations()`.

Gdy opcja jest `ON`, podpis `STREAM_ADD` może kanonizować kolejność operandów.

Gdy opcja jest `OFF`, kolejność operandów musi zostać zachowana. Wtedy:
- `A+B` i `A+B` nadal mogą zostać uznane za identyczne;
- `A+B` i `B+A` nie mogą zostać uznane za równoważne.

Nie wyłączaj całego współdzielenia `SELECT` przy wyłączaniu przemienności. Musi być możliwe odróżnienie syntaktycznego współdzielenia identycznych planów od współdzielenia wynikającego z prawa przemienności.

Konfiguracja:

    RDB_OPT_SHARE_EQUIVALENT_SELECTS=OFF
    RDB_OPT_COMMUTATIVE_ADD=ON

jest niepoprawna. CMake powinien zakończyć konfigurację czytelnym `FATAL_ERROR`, ponieważ przemienność nie jest wykorzystywana bez współdzielenia równoważnych `SELECT`.

D. RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES

Steruje wywołaniem:

    compiler::factorMatchedHashTimeMoves()

Po ustawieniu `OFF` kompilator nie może stosować przepisywania odpowiadającego regule:

    (A > i) # (B > k)  →  (A # B) > (i + k)

dla zgodnych przesunięć czasowych. Plan ma pozostać w postaci sprzed tego przebiegu.

2. Sposób przekazania opcji do C++

Przekaż wszystkie opcje jako makra o jawnej wartości `0` albo `1`.

W kodzie używaj:

    #if RDB_OPT_...

Nie używaj:

    #ifdef RDB_OPT_...

ponieważ makro zdefiniowane jako `0` nadal spełnia warunek `#ifdef`.

Preferuj definicje przypisane do właściwego targetu CMake zamiast niepotrzebnych definicji globalnych, o ile architektura obecnego CMake na to pozwala.

3. Warunkowe uruchamianie przebiegów kompilatora

W `compiler::compile()` zachowaj obecną kolejność etapów. Dodaj warunki kompilacji wyłącznie wokół badanych, opcjonalnych przebiegów:

    factorMatchedHashTimeMoves()
    deduplicateSubstrats()
    shareEquivalentSelectComputations()

Wyłączony przebieg:
- nie może zostać wywołany;
- nie może zmodyfikować `qTree`;
- nie może powodować kosztu wykonania ani warunku runtime.

Nie dodawaj zmiennych środowiskowych ani argumentów runtime sterujących tymi optymalizacjami. Są to przełączniki czasu budowania.

4. Nie wyłączaj etapów wymaganych dla poprawności

Nie dodawaj przełączników dla:

    extractIntermediateStreams()
    expandSchemaWildcards()
    resolveStreamIntervals()
    resolveFieldReferences()
    expandIndexWildcards()
    localizeFieldOffsets()
    computeRequiredCapacities()
    validateConstraints()
    applyCapacitiesToStreams()

Te etapy muszą działać we wszystkich konfiguracjach, ponieważ przygotowują wykonywalny plan albo sprawdzają jego poprawność.

5. Zachowaj obecną semantykę konfiguracji domyślnej

Konfiguracja ze wszystkimi opcjami `ON` musi zachowywać aktualne działanie systemu.

Nie refaktoryzuj algorytmów optymalizacyjnych poza zmianą konieczną do rozdzielenia:
- syntaktycznej identyczności planów `SELECT`;
- równoważności wynikającej z przemienności `STREAM_ADD`.

W szczególności przy `RDB_OPT_COMMUTATIVE_ADD=ON` obecny sposób kanonizacji argumentów dodawania ma zostać zachowany.

6. Dodaj identyfikację konfiguracji binarki

Binarka musi umożliwiać jednoznaczne odczytanie wartości wszystkich czterech przełączników.

Wykorzystaj istniejący mechanizm `--version` lub informacji o buildzie, jeśli istnieje. Jeśli takiego mechanizmu nie ma, dodaj minimalny argument:

    xretractor --optimizer-build-info

Wynik powinien być stabilny i możliwy do przetwarzania automatycznie, np.:

    RDB_OPT_DEDUP_SUBSTRATES=ON
    RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON
    RDB_OPT_COMMUTATIVE_ADD=ON
    RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=ON
    RDB_BENCH_PROBE=OFF

Polecenie ma wypisać konfigurację i zakończyć program bez uruchamiania silnika.

Podczas konfiguracji CMake również wypisz wartości wszystkich opcji przez `message(STATUS ...)`.

7. Zachowaj niezależność sondy pomiarowej

Istniejąca opcja:

    RDB_BENCH_PROBE

jest instrumentacją pomiarową, a nie przełącznikiem optymalizacji.

Nie łącz jej logicznie z opcjami `RDB_OPT_*`. Musi być możliwe zbudowanie dowolnej poprawnej konfiguracji optymalizatora zarówno z:

    RDB_BENCH_PROBE=OFF

jak i:

    RDB_BENCH_PROBE=ON

Sonda planu nie może błędnie sugerować, że deduplikacja została wykonana, gdy `RDB_OPT_DEDUP_SUBSTRATES=OFF`.

8. Zabezpiecz konfigurację budowania

Uwzględnij problem trwałych wartości w cache CMake.

Zwykły produkcyjny build wykonywany przez `scripts/buildrdb.sh release` powinien jawnie przywracać:

    RDB_OPT_DEDUP_SUBSTRATES=ON
    RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON
    RDB_OPT_COMMUTATIVE_ADD=ON
    RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=ON
    RDB_BENCH_PROBE=OFF

Tak samo zabezpiecz produkcyjne pakowanie, jeśli korzysta z istniejącego cache.

Nie dodawaj jeszcze profili eksperymentalnych typu `BASE`, `STRUCT`, `COMM`, `SHIFT` i `ALL`. Dobór kombinacji należy do późniejszego procesu ablacji. Kod ma jedynie zapewnić niezależne przełączniki.

9. Dodaj testy infrastruktury

Dodaj lub dostosuj testy tak, aby sprawdzały:

A. Konfigurację domyślną:
- wszystkie dotychczasowe testy nadal przechodzą;
- zachowane jest dotychczasowe przepisywanie przesunięć;
- zachowana jest deduplikacja substratów;
- zachowane jest współdzielenie równoważnych `SELECT`;
- `A+B` i `B+A` są rozpoznawane jako równoważne tam, gdzie pozwalają na to obecne ograniczenia.

B. Wyłączenie deduplikacji substratów:
- identyczne substraty pozostają osobnymi elementami planu;
- wynik zapytania pozostaje semantycznie taki sam.

C. Wyłączenie współdzielenia `SELECT`:
- wspólny substrat `STREAM_SELECT_*` nie jest tworzony;
- publiczne `SELECT` wykonują osobne obliczenia;
- wyniki pozostają równoważne konfiguracji domyślnej.

D. Wyłączenie przemienności przy pozostawionym współdzieleniu:
- identyczne `A+B` i `A+B` mogą być współdzielone;
- `A+B` i `B+A` nie są współdzielone;
- wyniki pozostają semantycznie równoważne.

E. Wyłączenie faktoryzacji przesunięć:
- plan zachowuje osobne przesunięcia przed `#`;
- reguła `factorMatchedHashTimeMoves()` nie jest stosowana;
- wynik wykonania pozostaje równoważny konfiguracji domyślnej.

F. Informację o buildzie:
- raportowane wartości odpowiadają wartościom użytym podczas konfiguracji CMake.

Nie osłabiaj istniejących testów domyślnej konfiguracji. Testy zależne od konfiguracji mogą wykorzystywać te same makra `RDB_OPT_*`.

10. Zweryfikuj reprezentatywne konfiguracje

Zbuduj i sprawdź przynajmniej następujące konfiguracje, każdą w osobnym katalogu budowania:

1. Wszystko włączone.
2. Wszystko opcjonalne wyłączone.
3. Tylko `RDB_OPT_DEDUP_SUBSTRATES=OFF`.
4. `RDB_OPT_SHARE_EQUIVALENT_SELECTS=OFF` oraz `RDB_OPT_COMMUTATIVE_ADD=OFF`.
5. `RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON` oraz `RDB_OPT_COMMUTATIVE_ADD=OFF`.
6. Tylko `RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=OFF`.
7. Przynajmniej jedna konfiguracja z `RDB_BENCH_PROBE=ON`.

Sprawdź również, że CMake odrzuca:

    RDB_OPT_SHARE_EQUIVALENT_SELECTS=OFF
    RDB_OPT_COMMUTATIVE_ADD=ON

Uruchom pełny zestaw testów dla konfiguracji domyślnej oraz testy skupione na kompilatorze i równoważności wyników dla pozostałych konfiguracji.

Nie wykonuj jeszcze pomiarów wydajności, wielokrotnych powtórzeń, analizy statystycznej ani porównania wariantów. To będzie osobny etap badania.

11. Dokumentacja i zakończenie

Dodaj krótką dokumentację opcji budowania zawierającą:
- nazwę każdej opcji;
- wartość domyślną;
- kontrolowaną funkcjonalność;
- zależność między przemiennością i współdzieleniem `SELECT`;
- przykład konfiguracji CMake;
- ostrzeżenie o cache CMake;
- informację, że `RDB_BENCH_PROBE` jest niezależną instrumentacją.

Na końcu:
- uruchom formatowanie wymagane przez repozytorium;
- pokaż wyniki kompilacji i testów dla każdej sprawdzonej konfiguracji;
- pokaż `git diff --check`;
- przedstaw zwięzłe podsumowanie zmienionych plików;
- wyjaśnij wszelkie odstępstwa od wymagań;
- nie wykonuj commitowania ani pushowania na `master`; pozostaw zmiany do mojego przeglądu.