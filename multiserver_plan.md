# Wieloserwerowość xretractor — plan i stan prac

Dokument roboczy gałęzi `issue_238-multiserver`. Powstał po zamknięciu etapów 0, 1 i 2a, żeby
kolejny etap dało się zacząć od czystego kontekstu bez powtarzania rozpoznania.

---

## Zadanie do wykonania w nowej sesji

> **Cel:** E3 — dostarczenie zestawu zapytan dzialajacemu serwisowi bez recznego restartu.
> **Gotowe gdy:** ustalone z czlowiekiem; zakres nizej jest przeslanka, nie planem.
> **Pliki dotkniete:** do ustalenia.

Routing 2c jest zamkniety, wiec sciezka E3 ma juz komplet przeslanek: skompiluj plik, sprawdz
magistrale, przy kolizji nazw strumieni to jest wlasciciel tego DAG-u — dostarcz plik i
zrestartuj **jego** unit; przy rozlacznosci wystartuj nowa instancje. Restart, a nie IPC
ad-hoc: `getAdHoc` nie umie `RULE`, `STORAGE`, `SUBSTRAT` ani `PERCOUNTER`, a jego skutek jest
ulotny.

Otwarte, swiadomie nieobjete dotad:

- Kolizja sciezki licznika `:ROTATION` i katalogu `:STORAGE` nadal nie jest wykrywana.
- Strumien powolany zapytaniem ad-hoc **nie jest roszczony w magistrali**. Plan serwera rosnie
  o nazwe, ktorej rozlacznosc nie jest egzekwowana wobec pozostalych instancji.
- Pole `unit` (nazwa jednostki systemd) w slocie: wymaga wyniesienia `detectSystemdIdentity()`
  z `lockManager.cpp` i bumpu `layoutVersion`.

## Stan na dziś

| Etap | Zakres | Stan |
|---|---|---|
| 0 | Kolejność: blokada instancji przed obiektami IPC | **zrobione**, commit `acd07dc` |
| 1 | Nazwy obiektów IPC sparametryzowane nazwą serwera | **zrobione**, commit `acd07dc` |
| 2a | Tożsamość serwera: `--name`, `--autoname`, `xqry --server` | **zrobione**, commit `0533a11` |
| 2b | Magistrala `xrdbbus`: wykrywanie + unikalność strumieni | **zrobione**, commit `0cb845d` |
| 2c | Routing automatyczny w `xqry` | **zrobione**, niezacommitowane |

Po etapie 2b dwa serwery pracują równocześnie, a kolizja nazw strumieni jest wykrywana przy
starcie i kończy się odmową wskazującą właściciela. Po 2c klient nie musi już wskazywać serwera:
`xqry -s dstb` sam trafia do właściciela, a komenda, której nie da się przypisać do jednej
instancji, kończy się odmową z listą kandydatów.

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

### Układ segmentu (zbudowany)

Segment nazywa się `xrdbbus`, **862 272 B** (32 sloty × 26 928 B + nagłówek) — zmierzone
`ls -l /dev/shm/xrdbbus`. Kod w `src/retractor/lib/bus.{hpp,cpp}`.

```
[ magic "XRDBBUS" (u64) ]               wpisywane JAKO OSTATNIE przy tworzeniu segmentu
[ layoutVersion (u32) = 1 ]             niezgodność => praca bez magistrali
[ slotCount (u32) = 32 ]
[ slotSize (u32) + reserved (u32) ]     zmiana POJEMNOŚCI bez bumpu wersji => odmowa
[ pthread_mutex_t (robust, pshared) ]   używany przy roszczeniu i przy zwalnianiu slotu
[ slot[0..31] ]
    seq          (u32)          seqlock: nieparzysty = zapis w toku
    pid          (i32)
    startTime    (u64)          pole 22 z /proc/<pid>/stat
    streamCount  (u32)
    name         [40]
    queryFile    [256]
    streams      [128][208]
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
- **Pole `unit` (nazwa jednostki systemd) pominięte.** Nie jest potrzebne w 2b, a jego
  wypełnienie wymaga wyniesienia `detectSystemdIdentity()` z `lockManager.cpp` — to praca 2c
  albo E3, nie rezerwacja miejsca na zapas. Dodanie go później wymaga bumpu `layoutVersion`.
- **Pole `slotSize` dodane.** Numer wersji chroni przed zmianą znaczenia pól, a nie przed
  zmianą pojemności; pomyłka w tym miejscu kosztowała jeden przebieg testów podczas 2b.

### Trzy własności układu

1. **Jeden pisarz na slot.** Serwer zapisuje wyłącznie swój slot, więc między serwerami nie ma
   konkurencji o zapis.
2. **Odczyt bez blokady.** Czytelnik używa seqlocka: czyta `seq`, kopiuje slot, czyta `seq`
   ponownie; nieparzysty albo zmieniony = powtórz. `xqry --servers`, routing strumienia i
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
acquireLock()                              (etap 0)
kompilacja planu                           (launcher)
std::atexit(cleanup)
ipcServer.setServerName                    <— MUSI być tutaj, patrz niżej
xrdbbus: attach + claim nazw strumieni     <— tu odmowa przy kolizji
ipcServer.start()
publishLockInfo()
przetwarzanie
```

Roszczenie **po** kompilacji (znamy dopiero wtedy zbiór `q.id`) i **przed** startem transportu.

**`setServerName` musi stać przed roszczeniem, nie po nim.** To nie jest kosmetyka: odmowa
następuje już po `std::atexit(cleanup)`, a `cleanup()` kasuje obiekty IPC **według nazwy
instancji**. Z nazwą jeszcze nieustawioną instancja odprawiona z kwitkiem kasowała nazwy
historyczne — czyli segment, kolejkę komend i muteks *działającego* serwera bezimiennego.
Defekt powstał i został naprawiony w 2b; punkt (5) w `multiserver_uniqueness/uniqueness.sh`
jest na niego regresją i został zweryfikowany przez tymczasowe cofnięcie poprawki.

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
  nazwana↔bezimienna przechodziłaby niewykryta, a `xqry --servers` w 2c nie widziałby serwera
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
w `multiserver_routing/routing.sh` jest na to regresją: mierzy czas `--servers` nad segmentem
z samymi martwymi slotami i wymaga poniżej 1 s.

### Reguły rozstrzygania

| sytuacja | zachowanie |
|---|---|
| podano `--server X` | wygrywa zawsze, magistrala **nieczytana** |
| magistrala niedostępna albo 0 instancji | nazwa pusta = tryb historyczny |
| dokładnie 1 instancja | jej nazwa, **bez sprawdzania strumienia** |
| ≥2, `-s`/`-t <strumień>` | właściciel z magistrali; brak → kod `2` (`no_such_file_or_directory`) |
| ≥2, `-a` | wszystkie rozpoznane nazwy w jednej instancji → tam; inaczej kod `22` |
| ≥2, `-k`/`-d`/`-y`/`-l` | odmowa z listą kandydatów, kod `22` |

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

### Format `xqry --servers`

Jedna linia na instancję, pola oddzielone spacją, sortowane; instancja bezimienna jako
`(unnamed)`, brak pliku zapytań jako `-`:

```
alfa 249247 alfa.rql srca dsta
beta 249248 beta.rql srcb dstb
```

Brak żywych instancji: pusty stdout, jedna linia na stderr, kod `0`.

### Czego 2c nie objęło

- **Strumień powołany zapytaniem ad-hoc nie jest roszczony w magistrali.** Plan serwera rośnie
  o nazwę, której rozłączność nie jest egzekwowana wobec pozostałych instancji.
- `-w/--wait-server` nadal czeka na nazwy podane jawnie (albo historyczne), bo routing wymaga
  żywej instancji, a `-w` służy dokładnie sytuacji, w której jej jeszcze nie ma.

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
| `src/retractor/lib/bus.{hpp,cpp}` | magistrala `xrdbbus`: `claim()`, `release()`, `instances()`, `isProcessAlive()` |
| `src/retractor/lib/executorsm.cpp` | `run()` — kolejność startu i rejestracja w magistrali; `cleanup()` zwalnia slot |
| `src/retractor/launcher.cpp` | wczesny skan `--name` / `--autoname`, walidacja, nazwa pliku blokady |
| `src/qry/ipcClient.{hpp,cpp}` | klient zna nazwę instancji, z którą rozmawia |
| `src/qry/serverRouting.{hpp,cpp}` | czyste reguły routingu nad migawką magistrali; bez IPC |
| `src/qry/qryLauncher.cpp` | opcje `--server` / `--servers`, `resolveTarget`, `waitForServer` |
| `test/IntegrationTest_serial/serverlib.sh` | oprawa **jednoinstancyjna** — nie używać w testach wieloserwerowych |
| `test/IntegrationTest_serial/multiserver_named/` | wzorzec testu dwuserwerowego z własną bramką higieny |
| `test/IntegrationTest_serial/multiserver_uniqueness/` | 5 punktów kontrolnych unikalności; zabija serwer przez SIGKILL i sam sprząta po sobie |
| `test/IntegrationTest_serial/multiserver_routing/` | 6 punktów routingu; mierzy też czas `--servers` nad osieroconym segmentem |
| `test/UnitTest/test_bus.cpp` | 9 przypadków magistrali pod valgrindem; pracuje na segmencie `xrdbbus_ut`, nie na produkcyjnym |
| `test/UnitTest/test_serverRouting.cpp` | 15 przypadków reguł routingu; bez pamięci dzielonej i bez serwerów |

## Zgodność wsteczna — reguła obowiązująca do końca prac

Brak `--name` oznacza tryb historyczny: te same nazwy obiektów IPC, ten sam plik blokady, to samo
zachowanie. Kryterium przyjęte w etapie 1 i utrzymane w 2a brzmiało: **pełny `ctest` przechodzi bez
jednej zmiany w testach integracyjnych ani w `serverlib.sh`**. Utrzymać je w 2b i 2c — jeśli któryś
istniejący test wymaga poprawki, to sygnał, że coś przeciekło poza warstwę nazw.
