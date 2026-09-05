# Wieloserwerowość xretractor — plan i stan prac

Dokument roboczy gałęzi `issue_238-multiserver`. Powstał po zamknięciu etapów 0, 1 i 2a, żeby
kolejny etap dało się zacząć od czystego kontekstu bez powtarzania rozpoznania.

---

## Zadanie bieżącej sesji

> **Cel:** etap 2f — usunąć sześć usterek wykrytych w przeglądzie etapów 0–2e: parsowanie nazwy,
> stabilność blokad, kolejność rezerwacji przed szkodą, licznik rotacji przy dostarczaniu oraz
> poprawność routingu ad-hoc.
> **Gotowe gdy:** każdy punkt ma test regresyjny, skupiony zestaw multiserver przechodzi, pełne
> CTest Debug i Release przechodzą, a wynik `test_gate` jest zapisany niżej.
> **Pliki dotknięte:** `multiserver_plan.md`, launcher i executor, magistrala i manager blokad,
> routing `xqry` oraz odpowiadające im testy jednostkowe/integracyjne.

E3 pozostaje następnym etapem funkcjonalnym. Naprawy 2f mają najpierw zapewnić, że zwykły start
i istniejąca ścieżka dostarczania nie niszczą stanu innej instancji. Dopiero na tej podstawie E3
może wybierać właściciela DAG-u z magistrali zamiast z pojedynczego pliku blokady.

## Stan na dziś

| Etap | Zakres | Stan |
|---|---|---|
| 0 | Kolejność: blokada instancji przed obiektami IPC | **zrobione**, commit `acd07dc` |
| 1 | Nazwy obiektów IPC sparametryzowane nazwą serwera | **zrobione**, commit `acd07dc` |
| 2a | Tożsamość serwera: `--name`, `--autoname`, `xqry --server` | **zrobione**, commit `0533a11` |
| 2b | Magistrala `xrdbbus`: wykrywanie + unikalność strumieni | **zrobione**, commit `0cb845d` |
| 2c | Routing automatyczny w `xqry` | **zrobione**, commit `006c990` |
| 2d | Roszczenie nazw ad-hoc + zombie jako martwy slot | **zrobione**, commit `52a7d76` |
| 2e | Odsiew przed czynnościami nieodwracalnymi; licznik `:ROTATION` i `unit` w slocie (`layoutVersion` 2, segment `xrdbbus_v2`) | **zrobione**, commit `52a7d76` |
| 2f | Naprawy usterek 1–6 z przeglądu wieloserwerowości plus 8 znalezisk z przeglądu diffu | **zrobione**, commit `52a7d76`; Debug/Release 213/213, `test_gate` **zielona** |
| 2g | Usterka: cele wykresowe (`ninja dsp`, `ninja simple`) walczące o jedną tożsamość — `xplot.sh` na nazwanych instancjach | **zrobione**, commit `fbc51c8`; Debug/Release 213/213, `test_gate` **zielona** |
| 2h | Tryb pracy instancji w slocie i kolumna `MODE` w `xqry --servers` (`layoutVersion` 3, segment `xrdbbus_v3`) | **zrobione**, niezacommitowane; Debug/Release 213/213, `test_gate` **zielona** |
| 2i | Przestrzenie nazw dla równoczesnych testów integracyjnych: `RDB_NAMESPACE` + `bus::segmentName()` | **zrobione**, niezacommitowane; Debug/Release 213/213 przy `-j 4` i `-j 24`, `test_gate` **zielona** |
| 2j | Scalenie `IntegrationTest_serial` i `IntegrationTest_parallel` w jedno drzewo `test/IntegrationTest`, prefiks `it_` dla całości | **zrobione**, niezacommitowane; Debug/Release 213/213 przy `-j 4` i `-j 24` |

Po etapie 2b dwa serwery pracują równocześnie, a kolizja nazw strumieni jest wykrywana przy
starcie i kończy się odmową wskazującą właściciela. Po 2c klient nie musi już wskazywać serwera:
`xqry -s dstb` sam trafia do właściciela, a komenda, której nie da się przypisać do jednej
instancji, kończy się odmową z listą kandydatów. Po 2d rozłączność obejmuje także nazwy
powoływane w locie: ad-hoc z cudzą nazwą jest odrzucany, zanim ruszy plan serwera.

### Plan etapu 2f i dziennik postępu

Stan startowy: `issue_238-multiserver` na `006c990` plus etapy 2d/2e, wówczas jeszcze
niezacommitowane (weszły razem z 2f w `52a7d76`).
`git diff --check` oraz 16 skupionych testów multiserver/bus/IPC/lock/routing przechodzą przed
rozpoczęciem zmian.

| Punkt | Naprawa | Kryterium sukcesu | Stan |
|---|---|---|---|
| 1 | Zastąpić ręczny skan nazwy wczesnym parsowaniem zgodnym z Boost.Program_options; jedna wartość ma zasilać walidację, blokadę, magistralę i IPC. | `--name alfa`, `--name=alfa` i `-nalfa` wybierają tę samą instancję; `--autoname` z każdą formą `--name` jest odrzucane. | **zrobione**, test `it_multiserver_named` |
| 2 | Budować nazwę blokady z kanonicznej nazwy programu (`filename(argv[0])`), nigdy z pełnej ścieżki. | Wywołanie przez `PATH`, ścieżkę względną i bezwzględną wskazuje ten sam plik blokady i daje ten sam wynik `--status`. | **zrobione**, test `it_multiserver_named` |
| 6 | Nie usuwać stabilnego pliku blokady po `flock`; czyścić wyłącznie jego treść pod posiadaną blokadą, jeżeli jest to potrzebne diagnostycznie. | Test z trzema uczestnikami nie pozwala utworzyć dwóch niezależnie zablokowanych inode dla jednej nazwy. | **zrobione**, test `LockManagerFlock.releaseKeepsStableInode` |
| 3 | Przenieść przejęcie blokady instancji i atomowe roszczenie magistrali przed kasowanie artefaktów; utrzymać oba zasoby do końca pracy executora. | Dwa równoległe starty z kolidującym planem: dokładnie jeden startuje, przegrany nie zmienia artefaktów; drugi start z tą samą nazwą również niczego nie kasuje. | **zrobione**, punkty 9–10 `it_multiserver_uniqueness` |
| 4 | Włączyć znormalizowaną ścieżkę licznika do odsiewu wykonywanego przed `deliverQueryFile`; odmowa ma wskazywać właściciela tak samo jak końcowe `claim`. | Kolizja `:ROTATION` nie nadpisuje pliku zapytań i nie restartuje działającego serwisu. | **zrobione**, punkt 11 `it_multiserver_uniqueness` |
| 5 | Zastąpić ogólny tokenizer rozpoznawaniem rzeczywistych odwołań do strumieni w RQL; zachować pełną składnię `ID`, w tym `$`, i ignorować nazwy pól/funkcji/strumienia wynikowego. | `cell$0` routuje się poprawnie; kolizja nazwy funkcji lub pola ze strumieniem innego serwera nie daje fałszywego `CrossServer`; rzeczywiste dwa źródła nadal są odrzucane. | **zrobione**, test `ut_serverRouting` + `it_multiserver_routing` |

Kolejność realizacji: 1 → 2 → 6 → 3 → 4 → 5. Po każdym punkcie najpierw test najniższej
warstwy, następnie odpowiedni scenariusz integracyjny. Punkt 3 nie zostanie oznaczony jako gotowy
na podstawie samego odsiewu migawki: kryterium wymaga rezerwacji utrzymywanej przed pierwszą
czynnością destrukcyjną. Punkt 4 obejmuje istniejące dostarczanie do serwisu; pełny wybór serwisu
docelowego z magistrali nadal należy do E3.

#### Dziennik 2f — punkty 1, 2 i 6

- Wczesny parser używa teraz Boost.Program_options z `allow_unregistered`, więc nie utrzymuje
  drugiej, uboższej składni opcji. Pełny parser sprawdza dodatkowo, że otrzymał tę samą nazwę.
- Klucz blokady zaczyna się od `filename(argv[0])`; katalog uruchomienia i sposób znalezienia
  programu nie wchodzą już do tożsamości instancji.
- `FlockServiceGuard::releaseLock()` zwalnia blokadę i deskryptor, ale nie usuwa ścieżki. Test
  otwiera inode przed zwolnieniem pierwszego właściciela, blokuje go drugim deskryptorem i
  potwierdza, że trzeci guard nie dostaje nowego inode pod tą samą nazwą.
- Oprawy integracyjne usuwają własne stabilne pliki dopiero po zebraniu procesów. Skupiony zestaw
  16 testów multiserver/bus/IPC/lock/routing przechodzi po zmianie.

#### Dziennik 2f — punkty 3 i 4

- Launcher po kompilacji najpierw przejmuje blokadę instancji, następnie wykonuje atomowe
  `Bus::claim`, a dopiero po obu sukcesach kasuje artefakty. Obiekt magistrali z zajętym slotem
  jest przekazywany przez referencję do `executorsm::run` i żyje do końca serwera.
- Executor nie przejmuje już blokady ani nie tworzy drugiego roszczenia. Sprawdza niezmiennik
  wejściowy, rejestruje oba zasoby dla `cleanup()` i dopiero wtedy uruchamia IPC.
- Punkt 9 testu unikalności uruchamia drugi proces z tą samą nazwą i sprawdza, że artefakty
  pierwszego nadal istnieją. Punkt 10 uruchamia równolegle dwie różnie nazwane instancje z jednym
  strumieniem: zostaje dokładnie jeden właściciel, a jego artefakt pozostaje dostępny.
- Odsiew przed dostarczaniem do serwisu sprawdza teraz także znormalizowaną ścieżkę licznika.
  Punkt 11 trzyma syntetyczny lock `MODE: service`, podstawia atrapę `systemctl` i dowodzi, że
  konflikt nie zmienia pliku zapytań ani nie wywołuje restartu.

#### Dziennik 2f — punkt 5

- Routing ad-hoc nie zbiera już wszystkich identyfikatorów. Lekser pomocniczy czyta tylko
  wyrażenie `FROM`, zgodnie z alfabetem `ID` z `RQL.g4`, łącznie ze znakiem `$`.
- Literały, komentarze zagnieżdżone, reduktory strumieniowe i agregatory po kropce są pomijane;
  nazwa funkcji, pola i strumienia wynikowego nie może już stworzyć fałszywego `CrossServer`.
- Jednostkowo pokryte są `cell$0`, kolizje nazwy pola/funkcji/wyniku/agregatora, komentarze oraz
  rzeczywiste źródła z dwóch instancji. Istniejący test integracyjny routingu nadal przechodzi.

#### Dziennik 2f — przegląd przed commitem (3 września 2026)

Osiem znalezisk z przeglądu diffu, wszystkie naprawione w tej samej sesji:

- Sekcja „Układ segmentu" opisywała stan sprzed 2e (`xrdbbus`, `layoutVersion` 1, 862 272 B).
  Opisuje teraz stan bieżący, z historią w jednym akapicie zamiast w milczącej rozbieżności.
- `launcher.cpp`: komentarz przy ścieżce E3 wskazywał `exec.run` jako miejsce zgłoszenia braku
  blokady — blokada jest brana w `main()` od etapu 2f.
- `launcher.cpp`: komentarz przy kontroli zgodności parserów twierdził, że pełny parser nie ma
  opcji `name`; ma ją, tylko `vm` jej nie zawiera przy `--autoname`.
- `launcher.cpp`: plik zapytań publikowany w slocie był ścieżką jak wpisaną. Jest bezwzględny,
  tak samo jak w pliku blokady — slot czyta operator z innego katalogu roboczego niż serwer.
  Normalizację obu ścieżek robi teraz jeden `absolutePathOf`.
- `bus.cpp`: `unit` i `queryFile` były jedynymi polami obcinanymi bez śladu. Zostają obcinane
  (odmowa startu z powodu długiej nazwy unitu byłaby lekarstwem gorszym od choroby), ale
  z ostrzeżeniem w logu; podział na pola rozstrzygające i informacyjne jest opisany w `bus.hpp`.
- `serverRouting.cpp`: lekser składał tokeny do lowercase, a `RQL.g4` leksuje słowa kluczowe
  wielkościowo (`'FROM'|'from'`). Strumień nazwany `Min` albo `From` byłby pominięty w klauzuli
  FROM. Regresja: `serverRouting.sourceStreamsTreatMixedCaseKeywordsAsStreamNames`.
- `uniqueness.sh` punkt (10): sam brak procesu przegranego nie dowodził, że odpadł **na
  roszczeniu** — odpadłby tak samo po skasowaniu artefaktów. Test wymaga teraz komunikatu
  magistrali w logu przegranego i kompletu artefaktów zwycięzcy.
- `serverName.cpp`: wymiana całej listy `kSurnames` **nie należy do zakresu 2f**. Zmiana jest
  nieszkodliwa (64 pozycje, bez duplikatów, nic się do starych nazw nie odwołuje), ale idzie
  osobnym commitem — inaczej opis 2f nie opisuje własnego diffu.

#### Dziennik 2f — weryfikacja końcowa (3 września 2026)

- Skupiony zestaw 16 testów multiserver/bus/IPC/lock/routing: **16/16 przeszło**.
- Pełny CTest Debug: **213/213 przeszło** w 144,29 s.
- Pełny CTest Release, po instalacji bieżących binariów: **213/213 przeszło** w 72,81 s.
- `git diff --check` oraz kontrole watermarków zmienionych plików źródłowych i testowych
  w trybie ścisłym: **bez uwag**.

Przebieg po naprawach z przeglądu (ta sama sesja, drzewo z poprawkami):

- Pełny CTest Debug: **213/213 przeszło** w 141,18 s.
- Pełny CTest Release: **213/213 przeszło** w 70,10 s.
- `ninja -C build/Release test_gate`: **bramka zielona**, odcisk `src/`
  `7e97e566…6179b0e`. Testy mechanizmu H10 (5/5), obie kampanie po 10 010 planów, oba werdykty
  dokładne 9/9, **reżimy H10a zgodne z odniesieniem w próbie i poza próbą**, H9 korpus,
  samotesty i **84/84 kompilacji + 4/4 odrzucone mutanty**.

Wcześniejszy czerwony wynik bramki — brak wierszy z reżimem w `H10a` i pominięty poziom
H9 84/84 — pochodził **w całości z nieświeżych artefaktów bramki**, nie ze zmian 2f: profile
`build/K26v3-*` niosły odcisk starszej treści `src/`, a katalog `gate-work` został po
poprzednim przebiegu. Naprawa jest ta z pułapki nr 1 w `CLAUDE.md`: `rm -rf build/K26v3-*`,
`test/research_gate/h9/build_profiles.sh` (4 profile, ok. 7 min), `rm -rf` katalogu `gate-work`,
ponowna bramka. Warto to zapamiętać: przy brudnym drzewie **oba** poziomy potrafią zgasnąć
naraz i wyglądać jak dwie osobne awarie badawcze.

Kolejność ma znaczenie: profile buduje się **po** ostatniej zmianie w `src/`, bo ich odcisk
liczy się z treści drzewa. Przebudowa przed poprawkami byłaby pracą do wyrzucenia.

#### Usterka 2g — cele wykresowe walczą o jedną tożsamość (3 września 2026)

Objaw zgłoszony przez operatora: `ninja dsp` w jednym terminalu i `ninja simple` w drugim —
oba procesy giną. Odtworzone z logów, przyczyna jest jedna i leży **poza silnikiem**.

`scripts/xplot.sh` był napisany dla jednego serwera na maszynę: startował `xretractor` bez
`--name` (czyli w tożsamości historycznej), po `sleep 1` odpytywał go klientem i sprzątał
globalnym `xqry -k`. Przy dwóch celach naraz przebieg wyglądał tak:

1. `dsp` przejmuje blokadę `xretractor_service`; `simple` odpada na `acquireLock`
   (`launcher.cpp:589`) i nie startuje wcale;
2. `xplot.sh` celu `simple` nie sprawdza, czy jego serwer żyje — po `sleep 1` pyta o `str1`
   i trafia do **jedynej** żywej instancji, czyli do `dsp` (`routing::forSingleTarget`);
3. `not found: str1` kończy klienta, gnuplot się zamyka, a sprzątanie `xqry -k` — znów jedyna
   instancja — **zabija serwer celu `dsp`**;
4. w pierwszym terminalu klient traci serwer i jego skrypt też woła `xqry -k`, tym razem
   w pustkę (`IPC: No such file or directory`).

Zginął więc jeden serwer, nie dwa: drugi nigdy nie wstał, a jego skrypt posprzątał po cudzym.

Naprawa, w trzech miejscach:

- `scripts/xplot.sh` nadaje instancji nazwę wyprowadzoną z katalogu roboczego celu
  (`dsp`, `simple`, `rec205`, …; piąty argument nadpisuje), czeka na gotowość **własnej**
  instancji zamiast `sleep 1` i pilnuje przy tym, czy proces serwera jeszcze żyje, a każde
  `xqry` — z zapytaniem o strumień i z zabiciem — dostaje `--server "$NAME"`. Odsiew
  „ta nazwa już działa" stoi **przed** `rm -rf temp`, bo drugie uruchomienie tego samego celu
  kasowało magazyn działającej instancji, zanim silnik zdążył odmówić startu.
- `launcher.cpp` przy nieudanym `acquireLock` wypisuje na stderr, kto trzyma tożsamość
  (nazwa instancji, PID i plik zapytań właściciela z pliku blokady) oraz jak uruchomić drugą
  instancję. Dotychczasowe `Another instance is running, errno: …` z `lockManager.cpp` nie
  mówiło ani która to instancja, ani czyja; zostało usunięte, żeby ta sama odmowa nie
  pojawiała się na konsoli dwa razy. Diagnostyka niskiego poziomu zostaje w logu.
- `FlockServiceGuard::PeerInfo` czyta z pliku blokady także `PID:` — bez tego komunikat
  odmowy nie miał czym wskazać właściciela.

Regresja: `it_multiserver_no_clobber` wymaga teraz, żeby odmowa startu wskazywała PID
właściciela odczytany z pliku blokady (test oblewa po podmianie oczekiwanego napisu).
Sprawdzone także ręcznie, atrapą `gnuplot`: `dsp` i `simple` uruchomione równolegle rysują
jednocześnie i każdy zabija wyłącznie własny serwer, a drugie uruchomienie tego samego celu
odpada przed skasowaniem czegokolwiek.

Weryfikacja: CTest Debug **213/213** (230,40 s), CTest Release **213/213** (104,91 s),
`ninja -C build/Release test_gate` **zielona** po przebudowie profili (odcisk `src/`
`5d502ea3…0a767fa`) — H10 5/5, obie kampanie 10 010 planów, oba werdykty 9/9, reżimy zgodne
z odniesieniem, H9 korpus, samotesty i 84/84 kompilacji + 4/4 odrzucone mutanty. Podłoga
ablacyjna nie dotyczy tej zmiany: żaden przełącznik `RDB_OPT_*`, `compiler.cpp` ani reguły
ogona nie były ruszane.

#### Etap 2h — tryb pracy instancji w tabeli `--servers` (3 września 2026)

Tabela odpowiadała dotąd na „kto serwuje ten strumień", ale nie na „w jakim trybie to liczy".
Dwa serwery z tego samego `.rql` mogą pracować zupełnie inaczej — jeden z zegarem ściennym,
drugi offline — a operator nie miał tego skąd zobaczyć bez zaglądania do `ps`.

Tryb jest własnością URUCHOMIENIA, nie planu, więc jego źródłem jest linia poleceń, a nośnikiem
slot magistrali: nowe pole `modes` (maska bitowa) obok `queryFile` i `unit`. Pole jest czysto
informacyjne — magistrala nie podejmuje na jego podstawie żadnej decyzji, więc slot zapisany
zerem (instancja starszej binarki) opisuje się jako zwykły, a nie jako błąd.

Litery w kolumnie `MODE`, wypisywane w stałej kolejności i łączone, bo tryby nie są rozłączne:

| Litera | Opcja | Znaczenie |
|---|---|---|
| `N` | — | żaden z poniższych: zwykły przebieg taktowany zegarem |
| `R` | `--realtime` | SCHED_FIFO, `mlockall`, bezwzględne pobudki |
| `F` | `--no-clock` | pełna semantyka interwałów, bez czekania na zegar |
| `U` | `--until-eof` | stop w slocie, w którym pierwsze źródło wyczerpie dane |
| `M` | `--llimitqry` | limit iteracji pętli inny niż „bez limitu" |
| `X` | `--xqrywait` | przetwarzanie wstrzymane do pierwszego zapytania |
| `S` | `--service` lub jednostka systemd | proces, którego nie zatrzymuje się ręcznie |

Legenda jest ostatnim wierszem tabeli — litery nie są odgadywalne, a wiersz zaczyna się od
`MODE:`, więc nie pasuje do wzorców kolumnowych, którymi skrypty łapią wiersze instancji.
Szerokość każdej kolumny nadal wynika z najszerszej wartości, więc nazwa z `--autoname`
(`objective_galileo`) rozsuwa kolumnę `SERVER` zamiast rozjechać wiersz.

Zmiana układu slotu podnosi `layoutVersion` do 3 i — zgodnie z regułą niżej — nazwę segmentu do
`xrdbbus_v3`. Rozmiar slotu się nie zmienił (27 328 B): `modes` wszedł w wyrównanie, które slot
i tak niósł, więc segment ma nadal 874 560 B.

---

## Ustalenia, których nie trzeba powtarzać

Rozpoznanie zrobione i zweryfikowane pomiarowo w sesji z 2 września 2026. Poniższe fakty są
podstawą projektu 2b — nie trzeba ich odtwarzać.

### Gdzie fizycznie leżą obiekty IPC

Boost kładzie wszystko wprost w `/dev/shm` (zmierzone na uruchomionym serwerze):

```
1024208  RetractorQueryQueue        (~1 MB na serwer)
  65536  RetractorShmemMap
     32  sem.RetractorMapMutex
```

Enumeracja instancji jest więc wykonalna zwykłym `readdir("/dev/shm")`. Nazwy obiektów wylicza
`ipc::names(serverName)` z `src/include/constants.hpp`; pusta nazwa daje nazwy historyczne.

### Osierocone segmenty i koszt odpytywania

Po `kill -KILL` serwera segmenty **zostają w `/dev/shm` bezterminowo**, a plik blokady zostaje
(choć `flock` jest zwolniony). `xqry --hello` wobec osieroconego segmentu kończy się po
**3,008 s** komunikatem „server not found" — to dokładnie budżet klienta
(`kDefaultIpcClientResponseMaxFails` = 300 × `kClientResponsePollInterval` = 10 ms).

Wniosek dla projektu: **wykrywanie nie może polegać na odpytywaniu serwerów z timeoutem.**
Osierocony segment jest nieodróżnialny od żywego aż do wyczerpania budżetu, a przy N instancjach
szukanie nieistniejącego strumienia (czyli literówka — najczęstszy przypadek) kosztowałoby N × 3 s.
Dodatkowo `issue_215` celowo rozdzielił kody wyjścia `streamNotFound` i `serverNoResponse`;
wykrywanie przez timeout skleiłoby te dwie diagnozy z powrotem.

### Robust mutex działa na tej maszynie

`pthread_mutex` z `PTHREAD_PROCESS_SHARED | PTHREAD_MUTEX_ROBUST` w pamięci dzielonej —
zweryfikowane eksperymentem (dziecko ginie trzymając muteks):

```
lock po śmierci właściciela: rc=130 (EOWNERDEAD)
pthread_mutex_consistent: 0
kolejny lock: rc=0        → magistrala żyje dalej
```

Boost **nie** udostępnia tego atrybutu — `boost::interprocess::named_mutex` nie jest robust i
proces, który zginie trzymając go, zawiesza wszystkich pozostałych. Dlatego w części wspólnej
używamy surowego `pthread_mutex_t` umieszczonego w segmencie zarządzanym przez Boosta.

### Unikalność musi obejmować deklaracje

`rdb::StoragePaths` tworzy `<qryID>.desc` dla **każdego** wpisu planu
(`src/rdb/lib/storagePaths.cc:18`, katalog dokładany w linii 50), a `streamInstance` buduje magazyn także dla deklaracji
(`src/retractor/lib/streamInstance.cpp:26-32`). Dwa serwery z deklaracją o tej samej nazwie
w tym samym katalogu magazynu nadpisałyby sobie deskryptory. Kolizja jest więc **fizyczna**,
nie tylko logiczna.

Roszczenie obejmuje wszystkie `q.id` z `qTree` **z pominięciem dyrektyw** zaczynających się od
dwukropka (`:STORAGE`, `:ROTATION`).

### Rzeczy, których magistrala świadomie nie obejmuje

- Ścieżka pliku licznika z `:ROTATION` i katalog z `:STORAGE` nie są chronione rozłącznością nazw
  strumieni. Minimum na przyszłość: ostrzeżenie przy kolizji ścieżki licznika.
- Tor danych zostaje **prywatny per serwer**. Do magistrali trafiają wyłącznie metadane.
- Zużycie `/dev/shm` rośnie liniowo z liczbą serwerów: ~1 MB na kolejkę komend, a subskrypcja
  strumienia to `(1/interval) × 10 s × 1024 B` — przy 720 Hz to **~7,4 MB na jednego subskrybenta**.
  Na workerze Pi 400 to realne ograniczenie; rozważyć `ipcQueueBufferSeconds` per instancja.

---

## Magistrala `xrdbbus` — stan zrealizowany (etap 2b)

### Zasada nadrzędna

> We wspólnym obszarze **tylko POD i tablice o stałym rozmiarze**. Żadnego alokatora, żadnych
> kontenerów Boosta.

Powód jest konkretny: serwer zabity w trakcie `insert` do kontenera z alokatorem zostawia
niespójną stertę w pamięci dzielonej, a `EOWNERDEAD` wtedy nie pomaga — nie ma jak „naprawić"
półrozpiętego drzewa. Przy stałych slotach naprawa niezmiennika to jedna operacja: unieważnić
slot, którego dotyczył przerwany zapis.

### Układ segmentu (stan bieżący)

Segment nazywa się `xrdbbus_v3`, **874 560 B** (nagłówek 64 B + 32 sloty × 27 328 B) — zmierzone
`ls -l /dev/shm/xrdbbus_v3`. Kod w `src/retractor/lib/bus.{hpp,cpp}`.

W etapie 2b segment nazywał się `xrdbbus` i miał 862 272 B przy `layoutVersion` 1; etap 2e dołożył
do slotu `unit` i `counterPath`, co podniosło wersję układu do 2 i — zgodnie z regułą opisaną niżej
— przeniosło ją do nazwy segmentu. Etap 2h dołożył `modes`, czyli wersję 3; rozmiar slotu się nie
zmienił, bo nowe pole weszło w wyrównanie, które slot i tak niósł. Poniższa tabela opisuje układ
**aktualny**, nie historyczny.

```
[ magic "XRDBBUS" (u64) ]               wpisywane JAKO OSTATNIE przy tworzeniu segmentu
[ layoutVersion (u32) = 3 ]             niezgodność => praca bez magistrali
[ slotCount (u32) = 32 ]
[ slotSize (u32) + reserved (u32) ]     zmiana POJEMNOŚCI bez bumpu wersji => odmowa
[ pthread_mutex_t (robust, pshared) ]   używany przy roszczeniu i przy zwalnianiu slotu
[ slot[0..31] ]                         27 328 B (27 324 B pól + wyrównanie do 8)
    seq          (u32)          seqlock: nieparzysty = zapis w toku
    pid          (i32)
    startTime    (u64)          pole 22 z /proc/<pid>/stat
    streamCount  (u32)
    modes        (u32)          maska trybów pracy (R/F/U/M/X/S); pole informacyjne
    name         [40]
    queryFile    [256]          ścieżka bezwzględna; pole informacyjne, obcinane z ostrzeżeniem
    unit         [128]          jednostka systemd; pole informacyjne, obcinane z ostrzeżeniem
    counterPath  [256]          znormalizowana ścieżka licznika :ROTATION; za długa => odmowa
    streams      [128][208]     za długa nazwa => odmowa
```

Trzy odstępstwa od projektu wstępnego, każde z powodu:

- **32 sloty zamiast 64, 128 strumieni zamiast 64, nazwa strumienia 208 B zamiast 32 B.**
  Pole 32-bajtowe było błędem: `it_wide_from_names` wywraca się na nim natychmiast, bo
  `compiler::composeStreamName` produkuje nazwy w rodzaju
  `STREAM_ADD_STREAM_ADD_..._str01_..._str12` (ponad 130 znaków). Nazwa jest ograniczona od
  góry przez `substratNameBudget_C = 200` (`compiler.cpp:424`) — dłuższe kompilator zastępuje
  skrótem — więc 208 B pokrywa wszystko, co silnik potrafi wygenerować. Liczba strumieni ma
  zapas 2,5× wobec największego skompilowanego planu w repozytorium (53 węzły,
  `optimizer_ablation`).
- **Pole `unit` (nazwa jednostki systemd) pominięte w 2b, dołożone w 2e.** W 2b nie było
  potrzebne, a jego wypełnienie wymagało wyniesienia `detectSystemdIdentity()` z
  `lockManager.cpp` — więc zostało odłożone zamiast zarezerwowane na zapas. Etap 2e je dołożył
  razem z `counterPath` i zapłacił za to zapowiedzianym bumpem `layoutVersion` z 1 na 2.
- **Pole `slotSize` dodane.** Numer wersji chroni przed zmianą znaczenia pól, a nie przed
  zmianą pojemności; pomyłka w tym miejscu kosztowała jeden przebieg testów podczas 2b.

### Trzy własności układu

1. **Jeden pisarz na slot.** Serwer zapisuje wyłącznie swój slot, więc między serwerami nie ma
   konkurencji o zapis.
2. **Odczyt bez blokady.** Czytelnik używa seqlocka: czyta `seq`, kopiuje slot, czyta `seq`
   ponownie; nieparzysty albo zmieniony = powtórz. `xqry --bus`, routing strumienia i
   kierowanie `kill` nie mogą się o nic zaciąć, nawet gdy jakiś serwer właśnie kona.
3. **Muteks tylko przy roszczeniu.** Jedyna operacja wymagająca serializacji to „sprawdź
   rozłączność nazw strumieni ze wszystkimi żywymi slotami i zatwierdź swój" — raz przy starcie,
   kilkadziesiąt mikrosekund, na strukturze POD.

### Żywotność i sprzątanie

Slot jest żywy wtedy i tylko wtedy, gdy `/proc/<pid>` istnieje **i** `starttime` się zgadza.
Sam `kill(pid, 0)` nie wystarcza — PID-y są reużywane. Slot martwy jest wolny; kasuje go ten,
kto to zauważy. Bez demona i bez heartbeatów.

### Protokół odzyskiwania po `EOWNERDEAD`

1. `pthread_mutex_lock` zwraca `EOWNERDEAD`.
2. Przejrzeć sloty; slot z nieparzystym `seq` był w trakcie zapisu — wyzerować go w całości.
3. `pthread_mutex_consistent`, potem normalna praca.

Dlatego `release()` też bierze muteks, choć pisze wyłącznie własny slot: dzięki temu
„nieparzysty `seq` przy trzymanym muteksie" znaczy jednoznacznie „pisarz zginął", co jest
całą podstawą powyższej naprawy.

### Kolejność startu serwera (zrealizowana)

```
kompilacja planu                           (launcher)
acquireLock()                              <— przed artefaktami i IPC
xrdbbus: attach + claim zasobów            <— atomowa odmowa przed artefaktami
kasowanie własnych artefaktów              (tylko po obu sukcesach)
ipcServer.setServerName                    (executor)
std::atexit(cleanup)
ipcServer.start()
publishLockInfo()
przetwarzanie
```

Roszczenie następuje **po** kompilacji (dopiero wtedy znamy zbiór `q.id`) i **przed** pierwszą
czynnością destrukcyjną. Launcher przekazuje aktywną blokadę i zajęty slot do executora; odmowa
nie rejestruje `atexit` i nie dotyka IPC.

### Komunikat odmowy

Wskazuje właściciela, bo to jedyna informacja, która pozwala operatorowi działać:

```
xretractor: stream 'dst' is already served by instance 'alfa' (pid 178735)
xretractor: stream 'dst' is already served by the unnamed instance (pid 178901)
```

Kod wyjścia: `system::errc::device_or_resource_busy` — odrębny od `no_lock_available`, którym
kończy się nieudany `flock`, żeby dwie różne diagnozy nie skleiły się w jedną.

### Decyzje przyjęte w 2b

- **Instancja bezimienna (bez `--name`) też rejestruje się w magistrali.** Inaczej kolizja
  nazwana↔bezimienna przechodziłaby niewykryta, a `xqry --bus` w 2c nie widziałby serwera
  historycznego. Cena: segment powstaje przy każdym starcie, także w użyciu jednoserwerowym.
- **Przepełnienie slotu to odmowa startu, nie ciche obcięcie listy.** Slot z obciętą listą
  strumieni nie mógłby już odpowiadać na pytanie „czyja jest ta nazwa".
- **Niedostępna magistrala nie zatrzymuje serwera** (`ClaimStatus::Unavailable`): jeden
  uszkodzony segment nie może unieruchomić maszyny. Cena jest wypisana wprost w logu —
  rozłączność nazw nie jest wtedy egzekwowana.
- **Segmentu nikt nie kasuje.** Usunięcie go w chwili, gdy inna instancja trzyma odwzorowanie,
  zerwałoby jej magistralę. Naprawa uszkodzonego segmentu jest ręczna i komunikat mówi wprost
  `remove /dev/shm/xrdbbus`.

### Czego 2b nie objęło

- Wyścig inicjalizacji jest zamknięty (`create_only` → truncate → memset → mutex → magic
  zapisany na końcu zapisem zwalniającym; pozostali czekają na magic do 2 s), ale twórca
  segmentu, który zginie **między** `create_only` a zapisem magic, zostawia segment martwy na
  stałe. Okno to mikrosekundy, a wyjście z sytuacji jest w komunikacie. Automatycznej naprawy
  świadomie nie ma — wymagałaby drugiego protokołu wzajemnego wykluczania nad tym samym
  segmentem.
- Kolizja ścieżki licznika `:ROTATION` i katalogu `:STORAGE` nadal nie jest wykrywana.

---

## Etap 2c — routing w xqry (stan zrealizowany)

### Zasada nadrzędna

> Rozstrzygnięcie właściciela **nigdy nie odpytuje serwerów.** Żywotność instancji rozstrzyga
> `/proc`, nie timeout.

Powód jest wprost przeniesiony z pomiaru z 2 września: osierocony segment jest nieodróżnialny
od żywego aż do wyczerpania budżetu klienta (3,008 s), więc szukanie strumienia przez
odpytywanie kosztowałoby N × 3 s dokładnie w najczęstszym przypadku — literówce. Punkt (6)
w `multiserver_routing/routing.sh` jest na to regresją: mierzy czas `--bus` nad segmentem
z samymi martwymi slotami i wymaga poniżej 1 s.

### Reguły rozstrzygania

| sytuacja | zachowanie |
|---|---|
| podano `--server X` | wygrywa zawsze, magistrala **nieczytana** |
| magistrala niedostępna albo 0 instancji | nazwa pusta = tryb historyczny |
| dokładnie 1 instancja | jej nazwa, **bez sprawdzania strumienia** |
| ≥2, `-s`/`-t <strumień>` | właściciel z magistrali; brak → kod `2` (`no_such_file_or_directory`) |
| ≥2, `-a` | wszystkie rozpoznane nazwy w jednej instancji → tam; inaczej kod `22` |
| ≥2, `-k`/`-d`/`-l` | odmowa z listą kandydatów, kod `22` |

**Przy jednej instancji nie sprawdzamy strumienia celowo.** Diagnostyka „nie ma takiego
strumienia" należy wtedy do serwera, dokładnie jak przed 2c — i to jest powód, dla którego
żaden istniejący test integracyjny ani `serverlib.sh` nie wymagał poprawki.

### Decyzje przyjęte w 2c

- **Klient nie zakłada magistrali.** `Bus` dostał `createIfMissing`; `xqry` woła z `false`.
  Pusty segment 862 kB założony przez proces jednorazowy nie niesie żadnej informacji, a jego
  brak znaczy dokładnie tyle, że żaden serwer nie wystartował — stan normalny, nie awaria.
- **Kolejność warunków w `resolveTarget` odpowiada kolejności wysyłki w `main()`.** Gdyby się
  rozjechały, routing rozstrzygałby według innej komendy niż ta, która faktycznie poleci do
  serwera: `xqry -k -a "..."` zabija serwer, więc musi być rozstrzygany jak `-k`.
- **Tokenizacja ad-hoc pomija literały w apostrofach.** Bez tego napis `'dstb'` w wyrażeniu
  przekierowałby zapytanie do obcej instancji — a `getAdHoc` modyfikuje **plan** serwera
  (`executorsm.cpp:252`), więc trafienie w niewłaściwą instancję to trwały skutek uboczny,
  nie pomyłka do powtórzenia. Punkt (5) testu sprawdza planami przed i po, że odrzucony
  ad-hoc nie zostawił śladu.
- **`-k` rozróżnia się wyłącznie przez `--server`.** Forma pozycyjna `xqry -k <nazwa>` odpadła:
  `-k` nie przyjmuje wartości, a argument pozycyjny jest już zajęty przez `select`, więc
  `xqry -k str1` czytałoby się jak „zabij strumień str1".
- **Logika rozstrzygania jest czysta i wydzielona** (`src/qry/serverRouting.{hpp,cpp}`): pracuje
  na gotowej migawce, nie dotyka IPC. Dzięki temu 15 przypadków w `test_serverRouting` biegnie
  pod valgrindem bez pamięci dzielonej i bez startowania serwerów.
- **`bus.cpp` dokładana źródłowo do binarki `xqry`**, tak jak `appConfig.cpp` — zależy tylko od
  pthread/boost.interprocess/spdlog. Do biblioteki `qry` jej **nie** ma: testy jednostkowe
  linkują jednocześnie `qry` i `retractor`, a dwie kopie `bus.o` dałyby duplikaty symboli.

### Format `xqry --bus`

Tabela z nagłówkiem, wyrównanymi kolumnami i instancjami sortowanymi po nazwie; instancja
bezimienna jest pokazana jako `(unnamed)`, a brak pliku zapytań lub strumieni jako `-`.
Strumienie są rozdzielone przecinkami. Magistrala nadal przechowuje **bezwzględną** ścieżkę
pliku, tak samo jak plik blokady: wybór celu dostarczania z magistrali (E3) potrzebuje ścieżki,
którą da się otworzyć bez zgadywania `cwd`. Tylko widok operatorski skraca ją do rozpoznawalnego
ogona `.../<katalog>/<plik>`.

```
SERVER | PID    | QUERY              | STREAMS
-------+--------+--------------------+-----------
alfa   | 249247 | .../plans/alfa.rql | srca, dsta
beta   | 249248 | .../plans/beta.rql | srcb, dstb
```

Brak żywych instancji: pusty stdout, jedna linia na stderr, kod `0`.

### Czego 2c nie objęło

- `-w/--wait-server` nadal czeka na nazwy podane jawnie (albo historyczne), bo routing wymaga
  żywej instancji, a `-w` służy dokładnie sytuacji, w której jej jeszcze nie ma.

---

## Etap 2d — roszczenie nazw powołanych ad-hoc (stan zrealizowany)

### Dlaczego to nie jest kosmetyka

`getAdHoc` modyfikuje **plan działającego serwera** (`executorsm.cpp`, `importFrom` +
`addQueryToModel`), a `rdb::StoragePaths` zakłada `<qryID>.desc` dla każdego wpisu planu.
Nazwa dołożona w locie w drugiej instancji nadpisywała więc deskryptor cudzego strumienia
we wspólnym katalogu magazynu — dokładnie ta sama **fizyczna** kolizja, przed którą broni
etap 2b, tyle że wpuszczana tylnymi drzwiami. Skutek jest trwały, bo plan zostaje zmieniony.

### `Bus::claimAdditional` zamiast powtórnego `claim`

`claim()` zaczyna od `release()` (`bus.cpp`), bo powtórne roszczenie tej samej instancji ma
zastąpić jej slot, a nie kolidować sam ze sobą. W locie to jest niedopuszczalne: odmowa
zostawiłaby **działający** serwer bez slotu, czyli bez roszczenia także tych nazw, które już
obsługuje. Stąd osobna operacja:

- wymaga posiadanego slotu (bez niego `Unavailable`),
- pod muteksem sprawdza rozłączność wobec **cudzych** żywych slotów, własny pomijając,
- nazwy już obecne w slocie odfiltrowuje, więc powtórzone zapytanie przechodzi i slot nie
  rośnie o duplikaty,
- pojemność slotu sprawdza po odfiltrowaniu, przed jakimkolwiek zapisem,
- **odmowa nie ma żadnego skutku ubocznego** — slot zostaje bit w bit taki, jaki był.

### Miejsce sprawdzenia w `getAdHoc`

Po lokalnej kompilacji (dopiero wtedy znane są także węzły pośrednie, które kompilator dołożył
do planu) i **przed** `importFrom`, czyli przed jakąkolwiek zmianą planu serwera. Zbiór nowych
nazw wyznaczany jest dokładnie tą samą regułą co `compiler::importFrom`: węzły nie będące
dyrektywą, których plan serwera jeszcze nie zna.

Odmowa wraca do klienta kanałem `db`, więc `xqry -a` kończy się kodem `2` i komunikatem:

```
Rejected: stream 'dst' is already served by instance 'alfa' (pid 178735)
```

### Decyzje przyjęte w 2d

- **`Unavailable` przepuszcza zapytanie i loguje ostrzeżenie**, spójnie ze ścieżką startową:
  jeden uszkodzony segment nie może unieruchomić maszyny, a cena jest wypisana wprost.
- **Roszczenia nie cofamy, gdy dalszy `compile()` łańcucha albo `addQueryToModel` zawiedzie.**
  `importFrom` wstawił już te węzły do planu i istniejący kod ich stamtąd nie usuwa, więc
  roszczenie odpowiada rzeczywistej zawartości planu.
- **Sprawdzenie jest poza `core_mutex`.** Ad-hoc jest serializowany wątkiem komunikacyjnym,
  a jedynym innym pisarzem do planu jest znowu ad-hoc; branie muteksu magistrali pod
  `core_mutex` zatrzymywałoby pętlę przetwarzania bez potrzeby.

### Zombie liczy się jako martwy (kryterium żywotności, ta sama sesja)

Slot serwera zabitego, ale niezebranego przez rodzica, blokował swoje nazwy strumieni **dowolnie
długo**: zombie zachowuje `/proc/<pid>/stat` razem z niezmienionym `starttime`, więc oba warunki
żywotności były spełnione (sprawdzone eksperymentem). Kolejna instancja dostawała odmowę
wskazującą proces, który już nie przetwarza.

Odpytywanie serwerów przez `hello` odpadło z tego samego powodu co w 2b i 2c — 3,008 s budżetu
klienta na każdy martwy slot, i to na ścieżce startu. Rozwiązanie kosztuje zero: `processStartTime`
i tak czytał `/proc/<pid>/stat` i i tak przeskakiwał nad polem 3 w drodze do pola 22. `readProcStat`
zwraca teraz oba pola z tej samej linii, a `isProcessAlive` odrzuca stan `'Z'`.

Sprzątania nie trzeba było dopisywać: gdy `isProcessAlive` mówi „martwy", istniejąca ścieżka
w `claim()` i `claimAdditional()` sama woła `clearSlot` i uznaje slot za wolny.

Odrzucany jest **wyłącznie** `'Z'`. `'T'` (zatrzymany SIGSTOP-em) i `'D'` (nieprzerywalny sen) to
procesy żywe, które wznawiają pracę — uznanie ich za martwe wpuściłoby drugą instancję na ten sam
`<qryID>.desc`. Regresja: `test_bus` · `ZombieSlotIsFreeAgain` (fork, SIGKILL bez `waitpid`,
przejęcie nazwy przez rodzica, `waitpid` na końcu).

### Czego 2d nie objęło

- `xqry -a` bez `--server` przy ≥2 instancjach nadal rozstrzyga się regułami 2c (wszystkie
  rozpoznane nazwy w jednej instancji), a nazwa **nowa** z definicji nie należy do nikogo —
  routing wybiera więc instancję po nazwach źródeł, nie po nazwie tworzonego strumienia.
- Ścieżka dostarczania E3 w `launcher.cpp` wybiera cel z pliku blokady (`readPeerInfo`),
  a nie z magistrali: przy wielu instancjach trafia w tę spod blokady, nie we właściciela
  kolidujących nazw. To zadanie E3, nie 2d.

---

## Etap 2e — odsiew przed szkodą, licznik rotacji, `unit` w slocie

### Odsiew rozłączności w `launcher.cpp`

Roszczenie w `executorsm::run` przychodziło **za późno wobec dwóch czynności nieodwracalnych**,
które launcher wykonuje wcześniej:

1. dostarczenie zestawu do działającego serwisu — nadpisanie jego pliku zapytań i restart
   (`deliverQueryFile` + `restartService`),
2. skasowanie artefaktów strumieni planu (`dropArtifactFile`, gdy plan nie ma `:ROTATION`).

Instancja kolidująca i tak kończyła się odmową, więc **jedynym jej skutkiem była szkoda**: przy (1)
serwis zostawał w stanie failed z nadpisanym zestawem, przy (2) tracił dane działający serwer.
Punkt (2) nie jest teorią — ujawnił go nowy punkt (7) testu unikalności: `epsilon` kasowała
artefakty strumienia `adh`, powołanego ad-hoc w `gamma`, a `gamma` padała z
`FATAL: storage: internal record count mismatch ... in adh` i wieszała test na `wait`.

Etap 2f rozdzielił te ścieżki. Zwykły start używa atomowego `Bus::claim` przed kasowaniem
artefaktów. Migawka `findForeignOwner` pozostaje wyłącznie na ścieżce dostarczania do istniejącego
serwisu, gdzie pomija instancję docelową; obejmuje teraz także licznik przez
`findForeignCounterOwner`.

### Licznik `:ROTATION` w slocie

`PersistentCounter` wczytuje wartość przy starcie, a zapisuje dopiero w destruktorze, więc dwie
instancje na jednym pliku zapisują tę samą wartość i **gubią rotacje**, nadpisując sobie archiwa.
Licznik nie jest nazwą strumienia, więc rozłączność nazw go nie chroniła. Slot niesie teraz ścieżkę
licznika, a kolizja kończy się `ClaimStatus::CounterConflict` ze wskazaniem właściciela.

Ścieżkę normalizuje **wołający**, i to `absolute()` **przed** `weakly_canonical()`: plik licznika
przy pierwszym starcie jeszcze nie istnieje, a `weakly_canonical` nad nieistniejącą ścieżką
względną zwraca ją bez zmiany — czyli bez katalogu roboczego, o który w tej normalizacji chodzi.

Sam `:STORAGE` **nie** jest osobno chroniony i nie musi być: pliki są per nazwa strumienia, a te są
rozłączne.

### `unit` w slocie i `layoutVersion` 2

`detectSystemdIdentity()` wyniesiony z anonimowej przestrzeni `lockManager.cpp` do
`lockManager.hpp`. Slot niesie nazwę jednostki systemd, więc magistrala umie wskazać, którą
jednostkę zatrzymać, żeby zwolnić kolidującą nazwę.

Slot urósł o `unit[128]` i `counterPath[256]`, więc `layoutVersion` idzie z 1 na 2, a `slotSize`
w nagłówku i tak wyłapałby samą zmianę pojemności.

Pułapkę wdrożeniową, którą to za sobą ciągnie, rozwiązuje **wersja w nazwie segmentu** — patrz
niżej.

### Czego 2e nie objęło, a 2f domknęło

- Druga instancja o **tej samej** nazwie odpada teraz na blokadzie przed kasowaniem artefaktów.
- Dostarczanie planu z kolidującym licznikiem ma test integracyjny bez zależności od systemd:
  syntetyczny proces trzyma prawdziwy `flock`, a `systemctl` jest kontrolowaną atrapą.
- `--autoname` nadal losuje raz, bez ponowienia przy trafieniu w nazwę żywej instancji.

### Wersja w nazwie segmentu: `xrdbbus_v3`

Segment o starym układzie zostaje w `/dev/shm` po podmianie binarki, a instancja, która odmówi się
do niego podłączyć, **startuje bez egzekwowania rozłączności** — awaria jest cicha aż do pierwszej
kolizji. Nazwa niesie więc wersję układu i idzie w górę razem z `layoutVersion` przy każdej zmianie
układu slotu. Nowa binarka po prostu zakłada własny segment, stary zostaje nieużywanym śmieciem do
restartu maszyny. Zweryfikowane: pełny `it_multiserver` przechodzi przy obecnym w `/dev/shm`
segmencie o poprzedniej nazwie, bez żadnego ręcznego `rm`.

**Automatycznego kasowania świadomie nie ma.** Rozważony wariant „sprawdź, czy żyją inne instancje,
i jeśli nie — skasuj segment" ma dwie wady, obie w miejscu, w którym miałby pomóc:

1. **Nieczytelnego segmentu nie da się zapytać o żywotność.** Przy obcym układzie `pid` i
   `startTime` czyta się jako śmieć, więc źródłem prawdy musiałby być skan `/proc` — czyli coś
   spoza magistrali.
2. **Kasowanie jest wyścigiem, i to psującym cicho.** `shm_unlink` usuwa nazwę, ale istniejące
   odwzorowania żyją dalej: instancja, która skasuje segment tuż po tym, jak inna go założyła,
   doprowadza do stanu **dwóch segmentów**, w którym każda instancja widzi tylko siebie i
   rozłączność nazw przestaje obowiązywać — bez jednego komunikatu. Dzisiejsza odmowa podłączenia
   jest zła, ale **głośna**. Zamknięcie tego wyścigu wymagałoby `flock` obejmującego całe
   podłączenie, czyli drugiego protokołu wzajemnego wykluczania nad tym samym segmentem.

Podkreślenie zamiast kropki jest częścią kontraktu: obiekty IPC instancji nazywają się
`<obiekt>.<nazwa instancji>`, więc `xrdbbus.v2` wyglądałby jak obiekt instancji o nazwie `v2`
i wpadłby pod wzorce sprzątające postaci `/dev/shm/*.<nazwa>`. Regresja:
`test_bus` · `BusSegmentName.CarriesLayoutVersionAndAvoidsInstanceNamespace`.

## Etap 2i — przestrzenie nazw równoczesnych testów integracyjnych

### Skąd wziął się problem

Gwarancja z etapu 2b ma skutek uboczny w zestawie testów. Rozłączność nazw strumieni jest
własnością **maszyny**, a nie katalogu roboczego, więc dwa testy integracyjne uruchomione
równocześnie nie mogą użyć tej samej nazwy strumienia — drugi start kończy się
`ClaimStatus::Conflict`. Nazwy powtarzają się w testach masowo: `core0` w czternastu
katalogach, `src` w trzynastu, `dst` w jedenastu, `str1` w dziewięciu. Dlatego 67 testów
integracyjnych stało pod `RUN_SERIAL` i dawało ~67 s ściśle sekwencyjnego ogona.

Zmiana nazw strumieni w 72 plikach `.rql` była rozważona i **odrzucona**: nazwa strumienia
przenika oracle testów — `term.script` (`open str1`), nazwy pól we wzorcach (`INTEGER str1_0`),
listing `temp/` w `pattern-ls.txt`, wyjścia DOT — więc trzeba by regenerować wzorce, czyli
oddać ich wartość regresyjną za jeden przebieg. Do tego `it_wide_from_names` ma nazwy złożone
powyżej 130 znaków przy budżecie `substratNameBudget_C` = 200; prefiks mógłby przełączyć
kompilator na skracanie nazw i zmienić wynik.

### Mechanizm

Jedna zmienna środowiskowa `RDB_NAMESPACE` (`servername::environmentNamespace`) rozdziela
**wszystkie cztery** zasoby globalne dla maszyny naraz:

| Zasób | Jak przestrzeń nazw go rozdziela |
|---|---|
| plik blokady instancji | wartość staje się nazwą instancji → `xretractor_service.<ns>.lock` |
| obiekty IPC w `/dev/shm` | ta sama nazwa instancji → sufiks przez `ipc::names()` |
| segment magistrali | `bus::segmentName()` → `xrdbbus_v3_<ns>` |
| plik logu | osobny `TMPDIR` ustawiany razem z `RDB_NAMESPACE` |

`xqry` bez `--server` celuje w instancję przestrzeni nazw; jawny `--server` pozostaje nadrzędny.
Wartość niepoprawna **zatrzymuje** oba programy z komunikatem — zignorowanie jej po cichu
cofnęłoby równoległe uruchomienie na zasoby wspólne, a awaria ujawniłaby się jako kolizja
u niewinnego sąsiada.

### Pula szesnastu, a nie jedna przestrzeń na katalog

Segment magistrali ma 874 KB i z założenia nikt go nie kasuje, a `/dev/shm` w kontenerze CI ma
domyślnie 64 MB. Pięćdziesiąt kilka segmentów (po jednym na katalog) zmieściłoby się na styk,
a przepełnienie `/dev/shm` **nie wywraca testu**: daje `ClaimStatus::Unavailable`, który jest
fail-open, czyli po cichu zdejmuje izolację. Szesnaście przestrzeni to najwyżej 14 MB
niezależnie od liczby katalogów — zmierzone po przebiegu dokładnie tyle.

Przydział jest per katalog i niesie `RESOURCE_LOCK` na nazwie przestrzeni. Załatwia to dwa
wykluczenia naraz: testy jednego katalogu dzielą katalog roboczy i `temp/`, więc i tak nie mogą
biec obok siebie, a katalogi, które trafiły na ten sam slot puli, dzielą tożsamość instancji.

### Katalogi poza pulą

Pięć katalogów zostaje przy `RUN_SERIAL`, bo badają tożsamość globalną maszyny, a przestrzeń
nazw zmieniłaby tam przedmiot badania: cztery `multiserver_*` (własne nazwy `alfa`/`beta`,
instancja bezimienna, produkcyjny segment magistrali) oraz `issue6_adhoc`, zostawiony jako
**jedyny strażnik ścieżki historycznej** — bez niego instancja bezimienna, jej blokada i jej
obiekty IPC straciłyby pokrycie end-to-end.

Próbowany był środek słabszy — wspólny `RESOURCE_LOCK` zamiast `RUN_SERIAL`, bo z testem
w przestrzeni nazw taki test nie dzieli żadnego zasobu. Nie dał nic mierzalnego (94,65 s wobec
94,34 s), a `multiserver_routing` mierzy czas `--bus` z progiem 1 s, więc obciążenie
sąsiadów mogło mu tylko zaszkodzić. Została mocniejsza gwarancja.

### Usterka wykryta przy okazji

`Data/test-workflow.sh` liczył kolejki odpowiedzi wzorcem globalnym `/dev/shm/brcdbr*` przed
przebiegiem i po nim. Nazwa kolejki to `brcdbr.<instancja>.<klient>`, więc pod `ctest -j 24`
test widział klientów **innej** przestrzeni nazw i zgłaszał ich jako własny wyciek
(`LEAK: brcdbr queue count increased from 0 to 3`). Wzorzec jest teraz zawężony do własnej
przestrzeni. Bramka higieny w `serverlib.sh` sprawdza z tego samego powodu człon przestrzeni
zarówno na końcu nazwy (obiekty serwera), jak i w środku (kolejki klientów).

### Wynik pomiaru

| Konfiguracja | Przed | Po |
|---|---|---|
| Debug, `ctest -j 4` | 141,13 s | 94,4 / 95,9 / 94,3 s |
| Debug, `ctest -j 24` | — | 93,4 / 93,5 s |
| Release, `ctest -j 4` | ~70 s (zapis z 2g) | 44,9 / 42,9 s |
| Release, `ctest -j 24` | — | 32,1 / 31,7 / 31,7 s |

Wszystkie przebiegi 213/213. Sufit Debug to teraz **nie** testy integracyjne, tylko
`ut_h10aGate` (~61 s w jednym procesie) plus ~33 s ogona `RUN_SERIAL`, w tym siedem testów
jednostkowych operujących na bezimiennych obiektach IPC. Dalsze skracanie wymagałoby ruszenia
tamtych, a nie testów integracyjnych.

### Czego 2i nie objęło

- Testy jednostkowe (`ut_dataModel`, `ut_ipcServer`, `ut_rdb`, `ut_soperations`, `ut_xqry`)
  nadal używają bezimiennych obiektów IPC i zostają `RUN_SERIAL`. Nazwa instancji jest w nich
  wpisana w kodzie C++, więc zmienna środowiskowa do nich nie sięga.
- `IntegrationTest_parallel` zostało wtedy bez zmian: te testy pracują w trybie `-c`, nie biorą
  blokady i nie dotykają magazynu, więc równoległość miały już wcześniej. Etap 2j scalił je
  z drzewem głównym.
- Plik logu ścieżki bezimiennej nadal rośnie w `TMPDIR` bez rotacji (`xretractor.log` na tej
  maszynie ma 94 MB). Poza zakresem.

## Etap 2j — scalenie drzewa testów integracyjnych

Po 2i podział na `IntegrationTest_serial` i `IntegrationTest_parallel` przestał cokolwiek znaczyć:
oba drzewa biegną równolegle, a drzewo „parallel" i tak w sześciu przypadkach pracowało na danych
z drzewa „serial", tylko z drugiego `CMakeLists.txt`. Scalone w `test/IntegrationTest`, prefiks
`it_` dla całości (`pt_` znaczyło „parallel test" i po scaleniu nie miało desygnatu).

Przeprowadzka rozpadła się na trzy grupy:

- **6 katalogów bez własnych plików** (`Data`, `issue42_rule`, `issue56_timeshift`,
  `issue61_tmpmem`, `simple`, `simple_max`) — miały tylko `DATA_DIR` wskazujący na bliźniaka.
  Ich `add_test` przeszły do tamtego `CMakeLists.txt`, `DATA_DIR` zastąpił `CMAKE_CURRENT_BINARY_DIR`.
- **19 katalogów z własnymi danymi** — `git mv` w całości, zero kolizji nazw.
- **7 katalogów z pułapką średnika** — patrz niżej.

### Pułapka średnika była tu prawdziwą robotą

`Pattern1`, `Pattern2`, `Pattern3`, `Pattern5`, `issue113_meta_autocreate`,
`issue202_hash_shift_factorization` i `issue31_doc` miały `bash -c "set -e ; a ; b"`. Działało to
**wyłącznie dlatego**, że drzewo równoległe nie miało makra `add_test`: jest dodawane alfabetycznie
przed `IntegrationTest_serial`, więc makro jeszcze nie istniało. W jednym drzewie każdy z nich
przechodzi przez `_add_test(${ARGV})`, które tnie argument po wewnętrznych średnikach — powłoka
wykonałaby samo `set -e` i test byłby zawsze zielony, nic nie sprawdzając. To ta sama awaria, po
której powstał `harness_command_integrity`.

Pięć przypadków to proste łańcuchy — `;` zamienione na `&&`. Dwa wymagały czegoś więcej:
`issue202_hash_shift_factorization` ma asercje postaci `if grep ... ; then exit 1 ; fi`, których
nie da się zapisać bez średnika, więc logika trafiła do `verify.sh` z parą `must` / `must_not` —
zgodnie z regułą domową ze strażnika. `issue31_doc` dał się przerobić na `&&` po usunięciu
zawieszonego separatora na końcu łańcucha.

### Czym to sprawdzone

Sam zielony przebieg **nie jest** tu dowodem — awaria średnika objawia się właśnie zielenią.
Dlatego:

1. Zbiór nazw z `ctest -N` przed i po scaleniu **identyczny** (213, modulo `pt_` → `it_`) —
   żaden test nie zginął w przeprowadzce ani się nie zdublował.
2. `harness_command_integrity` zielony — po `-c` stoi dokładnie jeden argument.
3. Testy przerobione oblewają, gdy mają oblewać: zepsute `pattern.txt` wywraca `it_Pattern1`,
   zepsute oczekiwanie w `verify.sh` wywraca `it_issue202_hash_shift_factorization-matched`,
   i to **obiema** gałęziami — `must` i `must_not`.
4. Łańcuchy dochodzą do ostatniego kroku: `issue113_meta_autocreate` zostawia `out_meta.txt`
   (krok piąty z siedmiu), `issue31_doc` cztery pliki `*.out.2.svg` (krok ostatni).

### Skutek uboczny, korzystny

40 testów kompilacyjnych weszło pod `RESOURCE_LOCK` swojego katalogu. To **ostrzejsze** niż przed
scaleniem: `pt_simple-compile` pisał `out-compile.txt` do tego samego katalogu roboczego, w którym
`it_simple-run` prowadził swój przebieg. Czas nie ucierpiał — 110 testów w 16 przestrzeniach,
Debug `-j 4` nadal 94,4 s, Release `-j 24` 32,2 s.

## Protokół weryfikacji

Rytuał startu sesji z `CLAUDE.md`, w tej kolejności:

```bash
git status                                  # czysto albo tylko przekazany diff
ninja -C build/Debug cformat                # sformatuj drzewo JAK ZASTANE
ctest --test-dir build/Debug -R ut_ipc      # baza musi być zielona przed zmianami
```

Przed zamknięciem:

```bash
ninja -C build/Debug && ninja -C build/Debug install && ctest --test-dir build/Debug -j 4
ninja -C build/Release && ninja -C build/Release install && ctest --test-dir build/Release -j 4
ninja -C build/Release test_gate
```

Kontrola watermarków w trybie ścisłym **natychmiast po edycji każdego źródła**, nie przy commicie:

```bash
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue <plik>
```

Bramka ablacyjna **nie jest wymagana**, dopóki etap nie dotyka `compiler.cpp`, `SOperations.hpp`,
`computeStartupLatency` ani kodu za przełącznikami `RDB_OPT_*`. Etapy 0–2c jej nie dotykały.
Bramka badawcza jest wymagana od 2b wzwyż — te etapy zmieniają `src/`, więc profile H9 trzeba
przebudować, inaczej poziom 84/84 zostaje pominięty (pułapka 1).

---

## Pułapki potwierdzone w praktyce

Każda z nich kosztowała czas w sesji z 2 września 2026.

1. **`ninja test_gate` pomija poziom H9 84/84 po każdej zmianie w `src/`.** Profil DEFAULT jest
   zbudowany z odcisku treści `src/`. Pominięty poziom liczy się jako *nieuruchomiony*, nie jako
   zdany. Naprawa (~7 min):
   ```bash
   rm -rf build/K26v3-*
   bash test/research_gate/h9/build_profiles.sh
   rm -rf build/Release/test/research_gate/gate-work
   ninja -C build/Release test_gate
   ```
   Katalog `gate-work` trzeba skasować także przed **drugim** przebiegiem bramki — inaczej oblewa.

2. **`pkill -f xretractor` zabija własną powłokę**, bo wzorzec pasuje do linii poleceń procesu,
   który go uruchomił. Używać `pgrep -x xretractor` i zabijać po PID.

3. **Asercje na konkretne wartości strumienia są chybotliwe.** Źródła są czytane w pętli, a klient
   dołącza w dowolnym jej miejscu — ta sama komenda daje raz `11,21,31`, raz `41,51,61`. Sprawdzać
   **przynależność do zbioru**, nie konkretną trójkę.

4. **`std::println` na przekierowany stdout jest buforowane blokowo.** Wszystko, co ma być widoczne
   w trakcie życia procesu (np. nazwa instancji z `--autoname`), wymaga `std::fflush(stdout)`.

5. **Resztki magazynu w katalogu roboczym wywracają kolejny przebieg** komunikatem
   `FATAL: storage: internal record count mismatch`. Ręczne próby uruchamiać w świeżym katalogu.

6. **Składnia RQL**: `SELECT srca[0]+1 STREAM dsta FROM srca`. Nie ma `AS` przed `STREAM`.

7. **Nazwy strumieni generowane przez kompilator bywają bardzo długie.** `it_wide_from_names`
   ma węzeł o ponad 130 znakach. Każda struktura o stałym rozmiarze indeksowana nazwą strumienia
   musi mieć co najmniej `substratNameBudget_C = 200` znaków (`compiler.cpp:424`) — powyżej tego
   progu `composeStreamName` sam podmienia nazwę na skrót, więc 200 jest twardą górną granicą.

8. **Zmiana pojemności struktury w pamięci dzielonej bez zmiany numeru wersji jest niewidoczna.**
   Stary segment o innym rozmiarze slotu czyta się jako śmieć albo blokuje start na oczekiwaniu
   na rozmiar. Stąd pole `slotSize` w nagłówku `xrdbbus`. Przy ręcznych próbach po zmianie
   układu: `rm -f /dev/shm/xrdbbus`.

9. **`cmake .` kasuje zbudowane binarki testów jednostkowych.** Po każdym `cmake .` zrobić `ninja`
   przed `ctest`. Testy integracyjne uruchamiają binarkę **zainstalowaną** — przed `ctest` zawsze
   `ninja install`, i uwaga na to, który profil (Debug/Release) stoi w `~/.local/bin`.

---

## Mapa kodu

| Plik | Rola |
|---|---|
| `src/include/constants.hpp` | `ipc::names(serverName)` — nazwy obiektów IPC instancji |
| `src/retractor/lib/serverName.{hpp,cpp}` | generator nazw w stylu dockera + walidacja |
| `src/retractor/lib/lockManager.{hpp,cpp}` | `acquireLock()` (wyłączność) i `publishLockInfo()` (treść) — rozdzielone celowo |
| `src/retractor/lib/bus.{hpp,cpp}` | magistrala `xrdbbus`: `claim()`, `claimAdditional()`, `release()`, `instances()`, odsiew właścicieli i `isProcessAlive()` |
| `src/retractor/lib/executorsm.cpp` | `run()` przyjmuje aktywną blokadę i slot; `cleanup()` zwalnia oba po IPC |
| `src/retractor/launcher.cpp` | wczesny parser tożsamości, blokada i roszczenie przed artefaktami, nazwa IPC |
| `src/qry/ipcClient.{hpp,cpp}` | klient zna nazwę instancji, z którą rozmawia |
| `src/qry/serverRouting.{hpp,cpp}` | czyste reguły routingu nad migawką magistrali; bez IPC |
| `src/qry/qryLauncher.cpp` | opcje `--server` / `--bus`, `resolveTarget`, `waitForServer` |
| `test/IntegrationTest/CMakeLists.txt` | pula szesnastu przestrzeni nazw: `RDB_NAMESPACE` + `TMPDIR` + `RESOURCE_LOCK` per katalog; `IT_NO_NAMESPACE` wypisuje katalog z puli |
| `test/IntegrationTest/serverlib.sh` | oprawa **jednoinstancyjna** (jedna instancja na przestrzeń nazw) — nie używać w testach wieloserwerowych; ścieżka blokady i bramka higieny podążają za `RDB_NAMESPACE` |
| `test/IntegrationTest/multiserver_named/` | wzorzec testu dwuserwerowego z własną bramką higieny |
| `test/IntegrationTest/multiserver_uniqueness/` | 11 punktów kontrolnych unikalności, kolejności startu i dostarczania; sam sprząta procesy i pliki |
| `test/IntegrationTest/multiserver_routing/` | 6 punktów routingu; mierzy też czas `--bus` nad osieroconym segmentem |
| `test/UnitTest/test_bus.cpp` | 26 przypadków magistrali pod valgrindem; pracuje na segmencie `xrdbbus_ut`, nie na produkcyjnym |
| `test/UnitTest/test_serverRouting.cpp` | 17 przypadków reguł routingu; bez pamięci dzielonej i bez serwerów |

## Zgodność wsteczna — reguła obowiązująca do końca prac

Brak `--name` **i** brak `RDB_NAMESPACE` oznacza tryb historyczny: te same nazwy obiektów IPC, ten
sam plik blokady, to samo zachowanie. Kryterium przyjęte w etapie 1 i utrzymane w 2a brzmiało:
**pełny `ctest` przechodzi bez jednej zmiany w testach integracyjnych ani w `serverlib.sh`**.
Obowiązywało do 2h włącznie. Etap 2i znosi je świadomie i tylko w jedną stronę: testy dostają
przestrzeń nazw, żeby mogły biec równocześnie, ale **żaden plik wzorcowy ani `.rql` się nie zmienił**
— gdyby zmiana sięgnęła wzorców, znaczyłoby to, że coś przeciekło poza warstwę nazw. Utrzymać je w 2b i 2c — jeśli któryś
istniejący test wymaga poprawki, to sygnał, że coś przeciekło poza warstwę nazw.
