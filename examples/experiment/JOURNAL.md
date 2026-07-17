# Dziennik badawczy — eksperyment wydajnościowy RetractorDB

Reguła prowadzenia: patrz REQUIREMENTS.md (wpisy datowane, chronologiczne;
błędne hipotezy zostają w dzienniku — są częścią drogi badawczej, nie wstydem).
Sprzęt workera: Raspberry Pi 400 (4× Cortex-A72 @ 1,8 GHz), Ubuntu 24.04,
jądro 6.8.0-raspi-realtime (PREEMPT_RT), system i repo na karcie SD.

---

## 2026-07-16 — dzień 1: od wymagań do dwóch poprawek rdzenia silnika

### Rano — analiza wymagań i budowa infrastruktury

- Przegląd REQUIREMENTS.md wykazał niespójności: literówki w ścieżce skryptu
  startowego; pkt 19 wskazywał `ecg/e1_stats.py` jako "sondę", podczas gdy to
  tylko analizator CSV — właściwa sonda to kod `RDB_BENCH_PROBE` w
  `executorsm.cpp`, aktywowany zmienną `RDB_BENCH_CSV` (nie pisze nigdzie
  domyślnie); brak samplera temperatury/CPU/RAM wymaganego przez pkt 11.
- Zbudowano infrastrukturę: `start_supervisor.sh` (nadzorca), `worker/run_study.sh`
  (badanie na workerze), `lib/common.sh`, konfiguracje kampanii CSV.
  Decyzje: dwie osobne kampanie (rate, clients) zamiast siatki rate×clients;
  pełny fizyczny reboot workera między badaniami; jeden wspólny branch
  eksperymentu dla wszystkich kampanii (bez brancha per kampania).

### Falstarty infrastruktury (obie porażki pouczające)

1. **`conan: command not found`** — nieinteraktywna sesja SSH nie sourcuje
   `~/.bashrc` (guard na początku pliku), więc PATH do `~/.local/bin` i venv
   Pythona nie istniały. Fix: `ssh_worker` jawnie odtwarza środowisko.
2. **Badanie 1 ubite timeoutem (kod 124)** — wzór na timeout zakładał, że czas
   przebiegu zbiega do `samples/rate_hz`. Fałsz: bez `-t` pętla śpi względnie,
   a gdy compute przekracza budżet, dryf rośnie bez ograniczeń. Fix: timeout
   niezależny od nominalnej częstotliwości. Przy okazji: brak tożsamości git
   na workerze (commit padał) — skonfigurowano.

### Baseline (wyniki w `results/rate/rotated/01/`)

Algorytm: Pan-Tompkins QRS (`rec205-detect.rql`), 20 000 próbek, 1 klient xqry.

| Częstość | mediana E1 | p99 | max | budżet | max/budżet |
|---|---|---|---|---|---|
| 360 Hz | 7290,6 µs | 8884,6 | 14488,0 | 2777,8 µs | **521,6%** |
| 720 Hz | 7268,4 µs | 8863,6 | 21850,2 | 1388,9 µs | 1573,2% |
| 1080 Hz | 6127,2 µs | 7353,2 | 13049,5 | 925,9 µs | 1841,0% |

**Kluczowy wniosek dnia:** system nie dotrzymuje budżetu już przy natywnych
360 Hz — pytanie badawcze przesunęło się z "przy jakiej częstości pęknie"
na "dlaczego koszt interwału jest o 3 rzędy wielkości za wysoki".

Diagnoza pomocnicza: to NIE karta SD (compute_ns płaski przez cały przebieg,
artefakty `.desc`/`.meta` powstają przed pętlą pomiarową), NIE termika
(`vcgencmd get_throttled` = 0x0, temp. 36-43°C), NIE wysycenie 4 rdzeni
(load1 ≈ 1,7-2,3). Wąskie gardło: koszt jednordzeniowy w silniku.

Odkrycie metodyczne: **`/tmp` na tym Pi to ext4 na karcie SD, nie tmpfs** —
założenie pkt 17 wymagań było fałszywe na tym sprzęcie. Skrypty przeniesione
na `/dev/shm` (prawdziwy tmpfs); dodana reguła rotacji wyników.

### Hipotezy z analizy statycznej kodu (subagent)

Szacunek: ~280-300 operacji skalarnych na próbkę ≈ ~1 µs na A72; zmierzone
7,3 ms = 4000-9000× więcej → narzut implementacyjny, nie limit sprzętu.
Trzy hipotezy: (H1) rekonstrukcja Descriptora okna w `constructAgsePayload`
per interwał; (H2) `boost::rational` w agregacjach INTEGER; (H3) interpreter
wyrażeń per pole z alokacją stosu.

### Fix #1 — `constructAgsePayload` (H1), wyniki w `results/rate/` (przebieg 2)

Cache deskryptora okna per `lengthAbs` + eliminacja pośredniej alokacji.

| Częstość | mediana E1 (Δ vs baseline) | max (Δ) |
|---|---|---|
| 360 Hz | 6842,0 µs (**-6,2%**) | 9639,3 µs (**-33,5%**) |
| 720 Hz | 6554,3 µs (-9,8%) | 9934,8 µs (-54,5%) |
| 1080 Hz | 5531,8 µs (-9,7%) | 8835,5 µs (-32,3%) |

Wniosek: poprawa realna, ale głównie w ogonie (jitter alokacji); mediana
prawie nietknięta → H1 nie była dominującym kosztem. Lekcja: **nie zgadywać
dalej — zmierzyć.**

### Zwrot metodyczny: profilowanie zamiast hipotez (callgrind, lokalnie x86)

300 interwałów tego samego pipeline'u pod callgrindem:

- `Descriptor::Descriptor(const&)` 17,4% + `rebuildFieldMappings()` 14,5%
  + `getSizeInBytes()` 8,8% + kopie wektorów + malloc/free ≈ **60% instrukcji**.
- **H2 obalona** — `boost::rational` nieobecny w czołówce profilu (<0,4%).
- **H3 obalona co do skali** — `expressionEvaluator::eval` self ≈ 1,3%.

Przyczyna źródłowa: `payload::getItem` (const) nie mógł leniwie przebudować
cache mapowań na własnym deskryptorze, więc **kopiował cały deskryptor przy
każdym odczycie pola**, przebudowywał mapowania na kopii i ją wyrzucał —
oryginał pozostawał "dirty" na zawsze. Dla okna 180-elementowego: O(N²)
alokacji na interwał.

### Fix #2 — const-poprawność cache Descriptora (commit `5ab4833`)

`mutable` cache mapowań + const-owe metody odczytu + `getItem` bez kopii
+ `getSizeInBytes` cache'owane w tej samej przebudowie. 3 pliki.

- Weryfikacja lokalna: **5,11 mld → 1,85 mld instrukcji (-63,7%)** na
  identycznym przebiegu; testy 152/152 (unit pod valgrindem + integracyjne).
- Zastrzeżenie do dokumentacji: leniwy mutable cache nie jest thread-safe
  przy równoczesnym pierwszym odczycie z dwóch wątków; w praktyce mapowania
  są budowane w jednowątkowej fazie kompilacji planu.

### W toku (16:11-...)

Kampania weryfikacyjna fixu #2 na workerze (rebuild + rotacja do
`rotated/02` + 3 badania). Oczekiwanie: jeśli -64% instrukcji przełoży się
choć w połowie na czas, mediana E1@360Hz spadnie w okolice budżetu 2,78 ms.

### Weryfikacja sprzętowa fixu #2 (kampania 3, wyniki w `results/rate/`,
### poprzednie przebiegi zrotowane do `rotated/01` i `rotated/02`)

| Częstość | Baseline | Fix #1 | Fix #2 | Δ łącznie |
|---|---|---|---|---|
| 360 Hz | 7290,6 µs | 6842,0 µs | **4321,7 µs** | **-40,7%** |
| 720 Hz | 7268,4 µs | 6554,3 µs | **3797,1 µs** | -47,8% |
| 1080 Hz | 6127,2 µs | 5531,8 µs | **2868,3 µs** | -53,2% |

(mediany E1; p99@360Hz: 8884,6 → 4997,3 µs; przepustowość unpaced:
137 → 236 próbek/s @360Hz). Teza fixu #2 potwierdzona — największy
pojedynczy skok wydajności. Lokalny wynik callgrind (-63,7% instrukcji)
przełożył się na ok. -37% czasu na ARM — różnica zgodna z oczekiwaniem
(instrukcje ≠ cykle; na Pi względnie droższy pozostały ruch pamięciowy).
Budżet @360Hz nadal przekroczony: mediana = 156% budżetu.

### Odkrycie: pływająca częstotliwość CPU (zauważone przez prowadzącego)

Migawki stanu pokazują "CPU(s) scaling MHz" wahające się **67-94%**
między przebiegami (governor `ondemand`, brak pinowania). Dowód wpływu
na pomiar: mediana E1 *maleje* ze wzrostem częstości napływu
(4322 → 3797 → 2868 µs), choć praca na interwał jest identyczna — przy
1080 Hz pętla działa niemal bez uśpień, governor trzyma wysokie takty;
przy 360 Hz dominują uśpienia i takty opadają. Stosunek median
360Hz/1080Hz = **1,51 ≈ 1,8 GHz / 1,2 GHz**. Wniosek: bez przypięcia
governora częstotliwość jest niekontrolowaną zmienną zakłócającą;
dotychczasowe porównania pozostają ważne co do istoty (delty >> wahań,
mediana z ~20k interwałów), ale liczby publikowalne wymagają `performance`.
Przewidywanie do zweryfikowania: z governorem `performance` mediana
@360Hz ≈ 2,9 ms, tuż przy budżecie 2,78 ms.

### Wdrożono: pakiet kontroli środowiska pomiarowego (`run_study.sh`)

1. Governor `performance` na czas badania (przywracany po, także przy
   awaryjnym wyjściu przez trap EXIT); worker ma pełne bezhasłowe sudo.
2. `taskset -c 3` dla xretractor; klienci xqry i sampler metryk na 0-2.
   Pełna wyłączność rdzenia 3 wymaga `isolcpus` — celowo osobny krok.
3. Migawki stanu rozszerzone o governor, chwilowe częstotliwości per
   rdzeń (cur/min/max) i `vcgencmd get_throttled` — przyszłe migawki
   będą rozstrzygające, nie poszlakowe.
Przy okazji: przywrócono parzystość master↔experiment (cherry-pick
`786f589` fix /tmp→/dev/shm, który żył tylko na branchu eksperymentu —
naruszenie pkt 6 wymagań).

### Kampania 4: weryfikacja pakietu kontroli środowiska (wyniki w
### `results/rate/`, poprzedni przebieg zrotowany do `rotated/03`)

Migawki potwierdzają kontrolę: wszystkie 4 rdzenie `performance` @ 1800000
kHz, `throttled=0x0`. Wyniki (mediany E1):

| Częstość | Fix #2 (ondemand) | Fix #2 + kontrola środowiska |
|---|---|---|
| 360 Hz | 4321,7 µs | **2960,3 µs** |
| 720 Hz | 3797,1 µs | **2969,8 µs** |
| 1080 Hz | 2868,3 µs | **3024,6 µs** |

**Oba przewidywania trafione:** (1) prognoza ~2,9 ms @360Hz → zmierzone
2960,3 µs (błąd <2%); (2) mediany wyrównały się między częstościami
(rozrzut 2960-3025 µs = 2,2% wobec wcześniejszych 50%) — zależność od
częstości napływu była w całości artefaktem governora, nie własnością
silnika. Uwaga metodyczna: kontrola środowiska nie przyspiesza kodu —
usuwa zakłócenie pomiaru; rzeczywisty koszt interwału tego algorytmu na
A72 @ 1,8 GHz to ~2,96-3,02 ms.

Stan celu badawczego po dniu 1: mediana = 106,6% budżetu @360Hz
(baseline: 262%), p99 = 132%. Silnik jeszcze nie dotrzymuje natywnego
tempa 360 Hz, ale od "3,5× za wolno w medianie" doszliśmy do "6,6% nad
kreską". Łączny postęp: 7290,6 → 2960,3 µs (**-59,4%**), przepustowość
unpaced 137 → 334 próbek/s (×2,4).

### Fix #3 — warstwa metaData (faza rozpoczęta na polecenie prowadzącego)

Diagnoza (callgrind, kontynuacja profilu z fixu #2): `MetaIndexStore::readAll()`
wywoływane 129 067× na 300 interwałów (~430×/interwał). Ścieżka odczytu
`nullBitset` per rekord robiła DWIE pełne kopie indeksu na dostęp
(`locateRecord` + `getNullBitset` → `readAll()[idx]` kopiuje cały wektor,
żeby zaindeksować jeden element), a każdy zapis (`appendEntry`/
`overwriteLast` przy każdym dopisanym rekordzie) unieważniał cache,
wymuszając ponowny odczyt pliku + pełną deserializację (67 205×
`IndexRecord::deserialize` → `unpackBits`).

Poprawka (3 pliki, semantyka bez zmian): `readAll()` zwraca `const&`
zamiast kopii (mutujący wywołujący dostają kopię przez `auto`);
cache write-through — `appendEntry` dopisuje do cache, `overwriteLast`
podmienia ostatni wpis, `rewrite`/`saveHeader` ustawiają nową zawartość;
plik czytany i deserializowany wyłącznie przy pierwszym dostępie.

Weryfikacja lokalna: 1,851 mld → 1,251 mld instrukcji (**-32,4%** vs
fix #2; łącznie od baseline **-75,5%**). Warstwa metaData zniknęła z
czołówki profilu; profil jest teraz płaski (lider: memcmp 5,95%) —
koniec ery pojedynczych dźwigni. Testy 152/152.

Incydent metodyczny (odnotowany zgodnie z regułą dziennika): 2 testy
integracyjne (`it_service_idle-*`) padły także na czystym masterze —
przyczyną okazała się binarka Release zainstalowana przeze mnie o 16:08
do `~/.local/bin` (workflow projektu instaluje z build/Debug; Release
wycina logi SPDLOG_INFO ze stderr, na których polega test trybu
usługowego). Naprawa: ponowna instalacja z Debug → 152/152. Wniosek
operacyjny: na maszynie nadzorcy `ninja install` wykonywać z build/Debug;
build Release+probe jest właściwy wyłącznie na workerze.

### Kampania 5: weryfikacja sprzętowa fixu #3 (wyniki w `results/rate/`,
### poprzedni przebieg zrotowany do `rotated/04`)

**Cel pośredni osiągnięty — silnik po raz pierwszy dotrzymuje budżetu
natywnego tempa 360 Hz** (mediana ORAZ p99 poniżej 2777,8 µs):

| Częstość | K4 (fix #2 + środowisko) | K5 (fix #3) | mediana/budżet |
|---|---|---|---|
| 360 Hz | 2960,3 µs | **1984,5 µs** | **71,4%** ✓ |
| 720 Hz | 2969,8 µs | 2006,2 µs | 144,5% |
| 1080 Hz | 3024,6 µs | 1873,0 µs | 202,3% |

@360Hz: p99 = 2521,4 µs (90,8% budżetu) ✓; jedynie max 2850,6 µs
(102,6%) minimalnie ponad — ogon do adresowania przez isolcpus/-t.
Lokalne -32,4% instrukcji przełożyło się na -33% czasu (niemal 1:1 —
usunięta warstwa była memory-bound, jak przewidziano). Skumulowany
postęp dnia: mediana @360Hz 7290,6 → 1984,5 µs (**-72,8%**),
przepustowość unpaced 137 → 495 próbek/s (**×3,6**).

Odpowiedź na pytanie badawcze pkt 21 (stan po dniu 1): na Pi 400
@1,8 GHz analiza w rygorze czasu rzeczywistego jest osiągalna do
~490 Hz napływu (przepustowość unpaced ~495-536 próbek/s); 720 Hz
pozostaje poza zasięgiem bez dalszych optymalizacji silnika.

### Test dymny SCHED_FIFO (-t) na workerze — dwa nowe ustalenia

Mechanizm: capabilities na binarce (`setcap cap_sys_nice,cap_ipc_lock+ep`,
rekomendacja samego rtCheckAndPrint) zamiast sudo-root — proces biegnie
jako michal (IPC shm i pliki bez zmiany właściciela). Weryfikacja: RT
check [OK]×3, PREEMPT_RT aktywny. Przebieg 3000 próbek @360Hz,
governor performance, taskset -c 3.

**Ustalenie 1 — E1 mieści się w budżecie, ale pełny slot NIE:**
E1 mediana 1796 µs (64,7% budżetu), lecz wake_lag rośnie liniowo
+0,32 ms/interwał. Arytmetyka sondy: `e2e − wake_lag ≈ 3,0 ms` =
E1 (1,8) + **~1,2 ms nieopomiarowanej pracy slotu** (getAwaitedStreamsSet
+ boradcast + księgowość osi czasu w executorsm). 3,0 > 2,78 ms → oś
czasu ucieka. Wniosek: metryka E1 ukrywała narzut slotu; dopiero oś E2E
(sensowna tylko z -t) go ujawnia. **Nowy, precyzyjny cel optymalizacyjny:
narzut slotu poza processRows (~1,2 ms).**

**Ustalenie 2 — piki E1 to dyskretne zdarzenia ~42,8 ms:** dokładnie
5 pików na 3000 interwałów, zaskakująco stała wielkość (41,8-42,9 ms),
odstępy 0,9-2,6 s — sygnatura kernelowego housekeepingu na
nieizolowanym rdzeniu. To motywacja dla `isolcpus` (plan 2), nie dla
zmian w kodzie.

Obserwacja operacyjna: zawartość /dev/shm na workerze zniknęła między
19:56 a 20:20 (timer tmpfiles odpalił wcześniej, reboot wykluczony) —
przyczyna nieustalona; dane badań są bezpieczne (kopiowane do repo
w trakcie badania), ale warto obserwować.

Decyzja: RT throttling jądra (95%) pozostaje włączony jako zawór
bezpieczeństwa — przy 720/1080 Hz wątek FIFO liczy niemal ciągle;
pełne wyłączenie mogłoby zagłodzić rdzeń. Odnotować przy interpretacji
ogonów na tych częstościach.

Wdrożono w `run_study.sh`: idempotentny `setcap` (capabilities znikają
po każdym rebuildzie) + flaga `-t`.

### Kampania 6: pierwsze pełne dane z SCHED_FIFO (-t), wyniki w
### `results/rate/`, poprzedni przebieg zrotowany do `rotated/05`

E1 pod SCHED_FIFO (mediana / p99, µs):

| Częstość | K5 (bez -t) | K6 (-t) |
|---|---|---|
| 360 Hz | 1984,5 / 2521,4 | **1777,1 / 2190,1** |
| 720 Hz | 2006,2 / 2501,1 | **1768,4 / 2172,3** |
| 1080 Hz | 1873,0 / 2461,0 | **1763,0 / 2164,5** |

Mediany E1 wyrównane do 0,8% rozrzutu (1763-1777 µs) — najczystszy
pomiar w historii eksperymentu; SCHED_FIFO ściął p99 o ~13%.

**Kluczowy wynik — pełny koszt slotu jest stały i wynosi ~3,17 ms:**
dryf wake_lag na 20k interwałów: 8,2 s @360Hz (+0,41 ms/slot),
35,6 s @720Hz (+1,78), 44,7 s @1080Hz (+2,24) — wszystkie trzy dają
slot ≈ budżet + dryf ≈ **3,16-3,19 ms** (E1 1,77 + ~1,4 ms narzutu
getAwaitedStreamsSet/boradcast/oś czasu; więcej niż 1,2 ms z testu
dymnego, bo doszedł 1 klient xqry). Wniosek: **rzeczywista przepustowość
end-to-end silnika to ~315 slotów/s < 360 Hz** — wynik kampanii 5
(E1 w budżecie) był optymistyczny o narzut slotu; dopiero oś E2E
pokazuje pełną prawdę. Korekta odpowiedzi na pkt 21: utrzymanie osi
czasu 360 Hz wymaga zejścia z narzutem slotu ~0,4 ms.

Piki ~43 ms obecne w każdym badaniu (max 42981-43266 µs, stała
wielkość) — potwierdzenie sygnatury housekeepingu jądra; adresat:
isolcpus.

### Fix #4 — leniwe formatowanie wiersza w boradcast + priorytet FIFO 80

Atrybucja narzutu slotu (callgrind, kontynuacja): `boradcast` = 53,1%
instrukcji inclusive, z czego `printRowValue` = 53,09% — **więcej niż
całe processRows (45,1%)**. Przyczyna: `boradcast` formatował wiersz
(printRowValue: budowa boost::property_tree + serializacja wszystkich
pól) dla KAŻDEGO strumienia w KAŻDYM interwale, zanim sprawdził, czy
ktokolwiek subskrybuje — przy 17 strumieniach i 1 kliencie (detect_out)
16 z 17 sformatowanych wierszy szło do kosza. Fałszywy trop po drodze:
to nie prezenter ekranowy (flaga -r nic tu nie daje) — formatowanie
siedzi w torze IPC.

Poprawka (executorsm.cpp, jedna funkcja): formatowanie leniwe — dopiero
przy pierwszym subskrybencie danego strumienia; semantyka dla
subskrybowanych strumieni bez zmian. Weryfikacja lokalna: 1,251 mld →
0,590 mld instrukcji (**-52,9%** vs fix #3; skumulowane od baseline
**-88,5%**). Testy 152/152.

Druga zmiana (run_study.sh): `scheduling.rt_priority = 80` przez TOML
(--config) — domyślne 50 jest RÓWNE domyślnemu priorytetowi wątków IRQ
PREEMPT_RT, więc wątek przerwań nie był wywłaszczany przez xretractor;
to główny podejrzany o dyskretne piki E1 ~43 ms. 80 = maksimum bez
ostrzeżenia appConfig. Hipoteza do weryfikacji w kampanii 7: piki
znikną lub zmaleją.

Zanotowany przyszły kandydat (poza zakresem): `IPC::message_queue
mq(open_only, ...)` otwierane per strumień per interwał dla
subskrybowanych strumieni — cache uchwytu kolejki.

### Kampania 7 (pierwsze podejście) — wynik negatywny FIFO 80,
### cenny i udokumentowany

Badanie 1 z `rt_priority=80`: **maszyna workera przestała odpowiadać
po sieci na >15 s** — sesja SSH nadzorcy padła, kampania przerwana.
Mechanizm: xretractor @FIFO 80 > wątki IRQ @50; przy 65% zajętości
rdzenia 3 wątki przerwań (sieć/mmc) były wywłaszczane na tyle długo,
że system wszedł w degradację. Wątek skryptu na workerze dokończył
badanie mimo zerwanej sesji (dane ocalone commitem 42cb4c8).

Obserwacja z ocalonych danych: **pik E1 zmalał 43 → 25 ms** — hipoteza
"piki = wątki IRQ blokujące xretractor" potwierdzona kierunkowo; ale
E2E z tego przebiegu jest skażone degradacją systemu (dryf 24 s) i nie
nadaje się do porównań. Wniosek: eskalacja priorytetu ponad wątki
przerwań to zły kompromis (zamienia piki na głodzenie systemu) —
właściwy adres pików to izolacja rdzenia (isolcpus / IRQ affinity),
przy priorytecie 50. `run_study.sh` wraca jawnie do rt_priority=50.

### Kampania 7bis (priorytet 50 + fix #4) i diagnoza anomalii 3,2 ms

Wyniki (E1 mediana / max, µs): 360 Hz: 1947 / **2655 — PIERWSZY RAZ
CAŁY rozkład E1 z maksimum w budżecie (95,6%)**; 720 Hz: 1736 / 42148;
1080 Hz: 1731 / 42098. Piki ~43 ms zniknęły @360Hz, obecne przy
720/1080 — hipoteza "piki = wątki IRQ" osłabiona; korelują z nasyceniem
pętli, nie z samą obecnością IRQ.

**Anomalia**: narzut slotu poza processRows @360Hz wyniósł 3,0 ms
(zamiast oczekiwanych ~0,2 ms) i oś czasu dalej uciekała. Diagnoza
przebiegami ręcznymi (3000 próbek, ±klient, dekompozycja per slot):
bez klienta narzut = **9 µs** i oś czasu utrzymana idealnie (fix #4
działa); w momencie podpięcia klienta narzut skacze do **~3,2 ms
stałych na slot**. Winowajca: `IPC::message_queue mq(open_only, ...)`
konstruowane w KAŻDYM slocie dla subskrybowanego strumienia
(shm_open+mmap per emisja). Zagadka K6 też rozwiązana: tam klient
odpadał na starcie (pełna kolejka), więc narzut 1,4 ms był czystym
formatowaniem 17 strumieni; fix #4 przesunął timing, klient przeżywa
i płaci otwarcie kolejki co slot.

Przy okazji rozwiązana zagadka znikającego /dev/shm: to systemd-logind
z RemoveIPC=yes (domyślne w Ubuntu) — po zamknięciu ostatniej sesji
użytkownika system usuwa jego pliki shm. Dane kampanii bezpieczne
(kopiowane do repo w trakcie sesji); diagnostyka musi być
jednosesyjna.

### Fix #5 — cache uchwytów kolejek IPC (executorsm.cpp)

`id2Queue_Cache`: kolejka otwierana przy pierwszej emisji do klienta,
uchwyt trzymany do usunięcia kolejki (przepełnienie) lub re-rejestracji
klienta (wtedy jawny erase — stary uchwyt wskazywałby odlinkowany
obiekt). Wzorzec współdzielenia między wątkami identyczny jak
istniejącej mapy klientów. Testy 152/152 (integracyjne ćwiczą tor
xqry→kolejka end-to-end). Oczekiwanie: narzut slotu z klientem
~3,2 ms → rzędu dziesiątek µs + koszt formatowania jednego strumienia.

### Kampania 8: fix #5 zweryfikowany — OŚ CZASU 360 Hz UTRZYMANA
### END-TO-END (wyniki w `results/rate/`, poprzedni przebieg w `rotated/08`)

Badanie 1 (360 Hz, 20 000 próbek, 1 klient xqry, SCHED_FIFO 50,
governor performance, taskset):

| Metryka | wartość | budżet 2777,8 µs |
|---|---|---|
| E1 mediana / p99 / max | 1865 / 2243 / 2635 µs | **max w budżecie (94,9%)** |
| **wake_lag mediana / p99** | **22,0 / 357 µs** | płaski — oś czasu trzyma |
| **E2E mediana / p99** | **1932 / 2519 µs** | **obie W BUDŻECIE** |
| E2E p99,9 / max | 29,9 / 43,3 ms | ogon: piki housekeepingu (~0,1%) |

Przewidywanie fixu #5 trafione z nawiązką (prognoza: wake_lag "setki µs";
zmierzone: mediana 22 µs — czysty jitter planisty). **Pierwsze
publikowalne CDF E2E**: silnik z podpiętym klientem dotrzymuje pełnego
rygoru czasu rzeczywistego przy natywnych 360 Hz w medianie i p99;
pozostały ogon (p99,9+) to rzadkie piki ~30-43 ms — adres: isolcpus.
720/1080 Hz: nasycenie bez zmian (E1 1756/1792 µs > budżety) — dryf
liniowy zgodnie z fizyką.

Bilans dnia 1 (@360 Hz, mediana): E1 7290,6 → 1865 µs (**-74,4%**);
E2E: z bezużytecznego (dryf dziesiątki sekund) do **1932 µs w budżecie**;
przepustowość unpaced 137 → ~540 próbek/s (×3,9). Pięć poprawek rdzenia
(AGSE cache, Descriptor const-cache, metaData write-through, leniwe
formatowanie, cache kolejek IPC) — każda: profil → hipoteza → pomiar
na sprzęcie → dziennik.

### Kampania 9: izolacja rdzenia (isolcpus=3 nohz_full=3 rcu_nocbs=3)
### — 2 przebiegi (decyzja prowadzącego: dla testów izolacji wystarczą 2)

Wdrożono w cmdline.txt workera (backup .bak-20260716), migawki stanu
rozszerzone o /proc/cmdline. Wyniki @360Hz vs K8 (bez izolacji):

| Metryka | K8 (bez izolacji) | K9 (izolacja) |
|---|---|---|
| E1 mediana / max | 1865 / 2635 µs | 1888 / 2680 µs (+1,2%) |
| E2E mediana / p99 | 1932 / 2519 µs | 1953 / 2683 µs (w budżecie) |
| wake_lag p99,9 / max | 26,9 / 40,9 ms | 35,6 / 47,2 ms |

**Wynik negatywny dla hipotezy ogona**: izolacja NIE usunęła pików
~40 ms — przetrwały w wake_lag (zdarzenie uderza w fazę snu; E1 max
pozostaje czysty). To nie jest szum planisty ani housekeeping usuwany
przez isolcpus — kandydaci: zdarzenie maszynowe (mmc/firmware
VideoCore/magistrala). Ogon p99,9 ≈ 0,1% pozostaje opisywalnym
artefaktem platformy. Koszt izolacji: +1,2% E1 @360Hz (akceptowalny),
~+8% przy nasyceniu 720 Hz.

**Hipoteza H-izolacja (sformułowana przez prowadzącego):** koszt
poniesiony na izolację xretractor na osobnym rdzeniu zwraca się w
niezależności metryk procesu (E1/E2E/wake_lag) od liczby klientów
xqry działających na rdzeniach 0-2 — weryfikacja: kampania `clients`
(1→2→3 klientów @360 Hz): metryki xretractor nie powinny się
degradować ze wzrostem liczby klientów.

### Projekt testu kontrastowego dsp-simple-fir (@note w REQUIREMENTS.md)

Cel: określić maks. tempo napływu dla PROSTEGO filtru FIR — kontrast
badawczy z pełnym potokiem QRS (~14 etapów SELECT). Baza:
`test/IntegrationTest_parallel/dsp` (okno @(1,25) → mnożenie
element-po-element → `.sumc` → złączenie; 4 etapy), zmodyfikowana
zgodnie z metodyką eksperymentu i dyspozycjami prowadzącego:

1. **Zero artefaktów** (problem przepustowości SD): usunięte
   `STORAGE 'temp'`, wszystkie SELECT `VOLATILE`, praca w /dev/shm.
2. Źródło: `BYTE` z `/dev/urandom` (pamięć, nie SD), interwał 1/RATE.
3. **Odbiór jak w badaniach QRS**: 1 klient xqry → /dev/null,
   przypięty do rdzeni 0-2.
4. Środowisko identyczne z kampaniami: governor performance,
   taskset -c 3, SCHED_FIFO 50 (-t), sonda RDB_BENCH_CSV, 20k próbek.
5. **Eskalacja tempa z regułą stopu**: stopnie 1000 → 2000 → 3000 Hz;
   jeśli mediana E1 (rdzeń obliczeń) przekroczy budżet stopnia,
   kolejne stopnie NIE są testowane (dyspozycja prowadzącego).

Implementacja: `worker/run_fir_contrast.sh`; wyniki do
`results/fir-contrast/` (results.md + surowe probe_*.csv), commit
konwencją eksperymentu (amend + force-with-lease na wspólnym commicie).

### Kampania clients: H-izolacja POTWIERDZONA w rdzeniu,
### ogon E2E skaluje się z N (wyniki w `results/clients/`)

360 Hz, izolowany rdzeń 3, klienci na 0-2:

| Metryka | 1 klient | 2 klientów | 3 klientów |
|---|---|---|---|
| E1 mediana | 1890,9 µs | 1932,6 (+2,2%) | 1949,3 (+3,1%) |
| E1 max | 2761 µs | 2813 | 2781 |
| E2E mediana | 1956,2 µs | 2008,5 | 2028,8 (+3,7%) |
| E2E p99 | 2659 µs | 2800 | 4779 |
| E2E p99,9 | 37,5 ms | 65,9 ms | 105,5 ms |
| wake_lag mediana | 23,2 µs | 22,8 | 23,6 |

**Werdykt H-izolacji (hipoteza prowadzącego): potwierdzona dla
zachowania typowego** — mediany E1/E2E/wake_lag praktycznie niezależne
od liczby klientów (wzrost ≤3,7% przy potrojeniu N); izolacja
xretractor na rdzeniu 3 skutecznie ekranuje obliczenia od klientów na
0-2, a koszt izolacji (+1,2% E1) zwraca się w tej niezależności.
**Zastrzeżenie**: ogon E2E p99,9 skaluje się ~liniowo z N
(37,5→65,9→105,5 ms ≈ N × zdarzenie ~40 ms) — rzadkie zdarzenie
platformowe multiplikuje się w torze emisji do N klientów; do
odnotowania w paper (ograniczenie), osobne śledztwo opcjonalne.

Dodatkowo (dyspozycja): zapytanie testu FIR zapisane jako wersjonowany
plik `config/dsp-simple-fir.rql` (runner podmienia tempo sedem, jak
rec205-detect.rql w kampaniach QRS).

### Test dsp-simple-fir wykonany (wyniki w `results/fir-contrast/`)

| Stopień | budżet | E1 mediana | werdykt |
|---|---|---|---|
| 1000 Hz | 1000,0 µs | 392,4 µs | w budżecie ✓ |
| 2000 Hz | 500,0 µs | 398,2 µs | w budżecie ✓ |
| 3000 Hz | 333,3 µs | 384,3 µs | **przekroczony → stop** |

Reguła stopu zadziałała zgodnie z projektem. Wnioski kontrastu:
prosty FIR (4 etapy) kosztuje E1 ≈ 390 µs/próbkę — **4,8× mniej niż
pełny potok QRS** (~1890 µs, ~14 etapów), proporcjonalnie do liczby
etapów potoku. Maks. tempo napływu dla FIR na Pi 400: potwierdzone
w budżecie **2000 Hz**; granica teoretyczna (budżet = E1) ≈ 2550 Hz.
Kontrast do paper'a: ta sama platforma realizuje w rygorze RT prosty
DSP przy 2 kHz i złożoną detekcję QRS przy 360 Hz (natywne tempo EKG).

### Stan celów badawczych na koniec dnia 1 (pkt 21 + @note)

1. "Jak szybkie dane możemy analizować": QRS — 360 Hz utrzymane
   end-to-end (E2E p99 w budżecie), nasycenie ~490-530 Hz; prosty FIR —
   2000 Hz potwierdzone, granica ≈ 2550 Hz.
2. "Ilu klientów": 3 klientów bez wpływu na mediany (H-izolacja
   potwierdzona); ogon p99,9 skaluje się z N (ograniczenie platformy).
3. "Jakie opóźnienie wnosi xretractor end-2-end": @360 Hz mediana
   1956 µs, p99 2659 µs (z klientem, SCHED_FIFO, izolowany rdzeń).

### Plany (otwarte, do decyzji prowadzącego)

1. Śledztwo zdarzenia platformowego ~40 ms (ogon p99,9; przetrwało
   isolcpus; multiplikuje się z N klientów w torze emisji).
2. Podniesienie granicy nasycenia QRS (720 Hz): profil płaski — dalsze
   zyski wymagają zmian strukturalnych (np. mniej etapów pośrednich).
3. Eksperyment na SSD (pkt 18, zapowiedziany w wymaganiach).

### Zamknięcie dnia 1 — bilans (2026-07-16)

Dziewięć kampanii + test kontrastowy w jeden dzień; pięć poprawek
rdzenia silnika, dwie zmiany kontroli środowiska, dwa wyniki negatywne
i cztery rozwiązane zagadki poboczne. Pełna droga @360 Hz (mediany):

| Etap | E1 | E2E | oś czasu |
|---|---|---|---|
| Baseline (rano) | 7290,6 µs | bezużyteczne (dryf) | uciekała |
| Fix #1 AGSE cache | 6842,0 µs | — | — |
| Fix #2 Descriptor const-cache | 4321,7 µs | — | — |
| + governor performance + taskset | 2960,3 µs | — | — |
| Fix #3 metaData write-through | 1984,5 µs | — | — |
| Fix #4 leniwe formatowanie + SCHED_FIFO | 1947,2 µs | — | uciekała (kolejki) |
| Fix #5 cache kolejek IPC | 1865,4 µs | 1932 µs | **utrzymana** |
| + isolcpus (koszt) | 1890,9 µs | 1956 µs | utrzymana, N-niezależna |

Skumulowane: E1 **-74%**, E2E z niemierzalnego do **p99 w budżecie**,
przepustowość unpaced ×3,9, FIR-kontrast 2 kHz. Wyniki negatywne
(zachowane zgodnie z regułą dziennika): FIFO 80 (głodzenie systemu),
isolcpus vs ogon ~40 ms (przetrwał). Zagadki rozwiązane: pływający
governor, /tmp≠tmpfs na Pi, logind RemoveIPC czyszczący /dev/shm,
mechanizm dryfu K6/K7bis (kolejki IPC otwierane per slot).
Wszystkie dane: `results/{rate,clients,fir-contrast}/` + `rotated/01-08`
na branchu experiment/20260716; infrastruktura i dziennik na master.
