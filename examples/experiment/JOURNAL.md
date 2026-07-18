# Dziennik badawczy — eksperyment wydajnościowy RetractorDB

Reguła prowadzenia: patrz REQUIREMENTS.md (wpisy datowane, chronologiczne;
błędne hipotezy zostają w dzienniku — są częścią drogi badawczej, nie wstydem).
Sprzęt workera: Raspberry Pi 400 (4× Cortex-A72 @ 1,8 GHz), Ubuntu 24.04,
jądro 6.8.0-raspi-realtime (PREEMPT_RT), system i repo na karcie SD.

---

## 2026-07-15 — dzień 0: dobór systemu operacyjnego workera pod RT
## (wpis retrospektywny, dopisany 2026-07-17)

Droga dojścia do Ubuntu 24.04 z jądrem realtime — z dwiema ślepymi
uliczkami, które uzasadniają wybór.

### Punkt wyjścia: Debian 13 (trixie) na Pi 400

`xretractor --help -t` (wbudowany check RT) zgłaszał: [FAIL]
CAP_SYS_NICE i CAP_IPC_LOCK, [WARN] RT throttling, RLIMIT_MEMLOCK
i brak jądra PREEMPT_RT. Trzy pierwsze naprawione na miejscu
(`setcap cap_sys_nice,cap_ipc_lock+ep` na binarce — do powtórki po
każdym `ninja install`; `kernel.sched_rt_runtime_us` przez sysctl;
memlock w `/etc/security/limits.conf`). Został jeden WARN: jądro bez
PREEMPT_RT (`6.18.34+rpt-rpi-v8`, fork RPi).

### Ślepa uliczka 1: ręczne patchowanie PREEMPT_RT na forku RPi

- Oficjalne repo `raspberrypi/linux` utrzymuje gałęzie `-rt` tylko dla
  archiwalnych jąder (rpi-4.14.y-rt, rpi-4.19.y-rt) — nic w pobliżu 6.18.
- Upstream patch RT (kernel.org) istniał dla vanilla 6.18.37-rt6,
  a fork RPi to 6.18.34 z własnymi zmianami w schedulerze/IRQ/timerach —
  nałożenie patcha gwarantuje konflikty (`.rej`) w tych samych miejscach,
  ich ręczne rozwiązywanie to praca na dni bez gwarancji bootowalnego
  obrazu (stack kamer/display/USB specyficzny dla RPi).

Werdykt: odrzucone. Utrzymywanie własnego forka jądra jest nie do
obrony metodycznie (niepowtarzalne środowisko badawcze).

### Decyzja: migracja na Ubuntu (Server, nie Desktop)

Canonical dostarcza oficjalnie utrzymywane jądro realtime w wariancie
`raspi` (BCM2711 = rodzina RPi4, Pi 400 się kwalifikuje) przez Ubuntu
Pro (darmowy do 5 maszyn): `pro enable realtime-kernel --variant=raspi`.
Przewaga nad ręcznym patchem: integrację RT z gałęzią raspi robi i
testuje Canonical. Wybór wariantu **Server**: Desktop dokłada
kompozytor/GUI/tło sesji konkurujące o CPU z wątkiem RT (wprost
sprzeczne z celem migracji — niskim jitterem); worker i tak jest
headless po SSH, a narzędzia (`xretractor`/`xqry`/`xtrdb`) to CLI.

### Ślepa uliczka 2: Ubuntu 26.04 (najnowszy LTS)

Pierwszy flash: Ubuntu Server 26.04 (jądro `7.0.0-1009-raspi`,
PREEMPT_DYNAMIC). Dwie lekcje:

1. **WiFi nie wstało** — bug sterownika `brcmfmac` przy negocjacji
   SAE/WPA3 (pętla `CTRL-EVENT-ASSOC-REJECT status_code=16`); fix:
   wymuszenie czystego WPA2-PSK w netplanie
   (`auth: key-management: psk`).
2. **Wariant `raspi` jądra realtime okazał się dostępny wyłącznie dla
   24.04** — `pro enable realtime-kernel --variant=raspi` failował;
   `pro help realtime-kernel` opisuje wariant jako "24.04 Real-time
   kernel optimised for Raspberry Pi", wbrew zapowiedziom dokumentacji
   o wsparciu 26.04. Wariant `generic` na RPi jest wprost odradzany
   ("make your system unusable").

### Stan końcowy: Ubuntu Server 24.04 LTS + realtime kernel

Ponowny flash (Ubuntu Server 24.04, jądro bazowe `6.8.0-1047-raspi`,
WiFi wstało bez obejścia), następnie `pro attach` + `pro enable
realtime-kernel --variant=raspi`:

```
Linux pi400 6.8.0-2049-raspi-realtime #50-Ubuntu SMP PREEMPT_RT
/sys/kernel/realtime = 1
```

Po migracji powtórzony tuning RT z Debiana (setcap, throttling,
memlock) — od tego stanu startuje dzień 1.

**Uzasadnienie wyboru (podsumowanie):** Ubuntu 24.04 LTS Server to
jedyna kombinacja dająca na Pi 400 oficjalnie utrzymywane, testowane
jądro PREEMPT_RT bez własnego forka — powtarzalne środowisko badawcze
(wersja jądra przypięta pakietem), przy minimalnym szumie systemowym
(headless Server). Debian/RPi OS odpadł brakiem jakiejkolwiek ścieżki
do PREEMPT_RT poza nieutrzymywalnym patchowaniem; 26.04 odpadł brakiem
wariantu `raspi` jądra realtime (stan na 2026-07-15).

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

### Baseline (wyniki w `results_20260716/rate/rotated/01/`)

> **_NOTE:_** Artefakty: [results_20260716/rate/rotated/01/](results_20260716/rate/rotated/01/).
> Konwencja stała dla wszystkich kampanii rate: `study_01/02/03` =
> 360/720/1080 Hz; każde badanie zawiera `results.md` (opracowanie),
> `e1_probe.csv` (surowa sonda), `metrics.csv` (sampler temp/CPU/RAM)
> oraz migawki stanu `state_before.md` / `state_after.md`.

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

### Fix #1 — `constructAgsePayload` (H1), wyniki w `results_20260716/rate/` (przebieg 2)

> **_NOTE:_** Wyniki po rotacji w [results_20260716/rate/rotated/02/](results_20260716/rate/rotated/02/).

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

### Weryfikacja sprzętowa fixu #2 (kampania 3, wyniki w `results_20260716/rate/`,
### poprzednie przebiegi zrotowane do `rotated/01` i `rotated/02`)

> **_NOTE:_** Wyniki kampanii 3 po rotacji w [rotated/03](results_20260716/rate/rotated/03/);
> poprzednie przebiegi w [rotated/01](results_20260716/rate/rotated/01/)
> i [rotated/02](results_20260716/rate/rotated/02/).

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
### `results_20260716/rate/`, poprzedni przebieg zrotowany do `rotated/03`)

> **_NOTE:_** Wyniki kampanii 4 po rotacji w [rotated/04](results_20260716/rate/rotated/04/).

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

### Kampania 5: weryfikacja sprzętowa fixu #3 (wyniki w `results_20260716/rate/`,
### poprzedni przebieg zrotowany do `rotated/04`)

> **_NOTE:_** Wyniki kampanii 5 po rotacji w [rotated/05](results_20260716/rate/rotated/05/).

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
### `results_20260716/rate/`, poprzedni przebieg zrotowany do `rotated/05`

> **_NOTE:_** Wyniki kampanii 6 po rotacji w [rotated/06](results_20260716/rate/rotated/06/).

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

> **_NOTE:_** Artefakty jedynego ukończonego badania:
> [rotated/07/study_01](results_20260716/rate/rotated/07/study_01/).

Obserwacja z ocalonych danych: **pik E1 zmalał 43 → 25 ms** — hipoteza
"piki = wątki IRQ blokujące xretractor" potwierdzona kierunkowo; ale
E2E z tego przebiegu jest skażone degradacją systemu (dryf 24 s) i nie
nadaje się do porównań. Wniosek: eskalacja priorytetu ponad wątki
przerwań to zły kompromis (zamienia piki na głodzenie systemu) —
właściwy adres pików to izolacja rdzenia (isolcpus / IRQ affinity),
przy priorytecie 50. `run_study.sh` wraca jawnie do rt_priority=50.

### Kampania 7bis (priorytet 50 + fix #4) i diagnoza anomalii 3,2 ms

> **_NOTE:_** Wyniki po rotacji w [rotated/08](results_20260716/rate/rotated/08/).

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
### END-TO-END (wyniki w `results_20260716/rate/`, poprzedni przebieg w `rotated/08`)

> **_NOTE:_** Wyniki kampanii 8 po rotacji w [rotated/09](results_20260716/rate/rotated/09/);
> badanie 1: [rotated/09/study_01](results_20260716/rate/rotated/09/study_01/results.md).

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

> **_NOTE:_** Wyniki po rotacji w [rotated/10](results_20260716/rate/rotated/10/)
> (study_01 = 360 Hz, study_02 = 720 Hz).

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
`results_20260716/fir-contrast/` (results.md + surowe probe_*.csv), commit
konwencją eksperymentu (amend + force-with-lease na wspólnym commicie).

### Kampania clients: H-izolacja POTWIERDZONA w rdzeniu,
### ogon E2E skaluje się z N (wyniki w `results_20260716/clients/`)

> **_NOTE:_** Artefakty: [study_01](results_20260716/clients/study_01/results.md) /
> [study_02](results_20260716/clients/study_02/results.md) /
> [study_03](results_20260716/clients/study_03/results.md) = 1/2/3 klientów.

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

### Test dsp-simple-fir wykonany (wyniki w `results_20260716/fir-contrast/`)

> **_NOTE:_** Opracowanie: [results.md](results_20260716/fir-contrast/results.md);
> surowe przebiegi sondy:
> [probe_1000hz.csv](results_20260716/fir-contrast/probe_1000hz.csv),
> [probe_2000hz.csv](results_20260716/fir-contrast/probe_2000hz.csv),
> [probe_3000hz.csv](results_20260716/fir-contrast/probe_3000hz.csv).

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
Wszystkie dane: `results_20260716/{rate,clients,fir-contrast}/` + `rotated/01-08`
na branchu experiment/20260716; infrastruktura i dziennik na master.

> **_NOTE:_** Stan na moment zamknięcia dnia 1; po rotacji z 2026-07-17
> komplet przebiegów rate to [rotated/01-10](results_20260716/rate/rotated/) —
> patrz wpis niżej.

---

## 2026-07-17 — dzień 2

### Rano — rotacja wyników K8/K9 i uzupełnienie referencji w dzienniku

- Rotacja ręczna (prowadzący): wyniki kampanii 8 trafiły do
  [rotated/09](results_20260716/rate/rotated/09/), wyniki kampanii 9 (`study_01`,
  `study_02` + README kampanii) do [rotated/10](results_20260716/rate/rotated/10/);
  katalog roboczy `results_20260716/rate/` opróżniony przed kolejnymi przebiegami.
- Dziennik uzupełniony o względne odnośniki do artefaktów w miejscach,
  gdzie wpisy opisują konkretne badania. Mapowanie kampania → katalog
  zweryfikowane danymi (mediany E1 study_01 zgodne z tabelami wpisów
  dnia 1, daty README kampanii zgodne z chronologią dziennika):

| Wpis dziennika (dzień 1) | Artefakty |
|---|---|
| Baseline | [rotated/01](results_20260716/rate/rotated/01/) |
| Fix #1 — AGSE cache | [rotated/02](results_20260716/rate/rotated/02/) |
| K3 — fix #2 (ondemand) | [rotated/03](results_20260716/rate/rotated/03/) |
| K4 — kontrola środowiska | [rotated/04](results_20260716/rate/rotated/04/) |
| K5 — fix #3 (metaData) | [rotated/05](results_20260716/rate/rotated/05/) |
| K6 — SCHED_FIFO (-t) | [rotated/06](results_20260716/rate/rotated/06/) |
| K7 — FIFO 80 (wynik negatywny, 1 badanie) | [rotated/07](results_20260716/rate/rotated/07/) |
| K7bis — priorytet 50 + fix #4 | [rotated/08](results_20260716/rate/rotated/08/) |
| K8 — fix #5, oś czasu utrzymana | [rotated/09](results_20260716/rate/rotated/09/) |
| K9 — isolcpus, 2 przebiegi | [rotated/10](results_20260716/rate/rotated/10/) |
| clients — H-izolacja | [results_20260716/clients/](results_20260716/clients/) (study_01/02/03 = 1/2/3 klientów) |
| FIR-kontrast | [results_20260716/fir-contrast/](results_20260716/fir-contrast/) |

Konwencja w kampaniach rate: `study_01/02/03` = 360/720/1080 Hz
(wyjątki: `rotated/07` — tylko `study_01`, kampania przerwana;
`rotated/10` — `study_01/02` = 360/720 Hz, decyzja o 2 przebiegach).
Zawartość każdego badania: `results.md` (opracowanie i werdykt),
`e1_probe.csv` (surowa sonda E1/E2E/wake_lag), `metrics.csv`
(temp/CPU/RAM), `state_before.md` / `state_after.md` (migawki stanu
workera — od kampanii 4 z governorem i częstotliwościami per rdzeń,
od kampanii 9 z /proc/cmdline).

### Sekcja Performance Evaluation artykułu wypełniona wynikami

Rozdział 7 main-debs.tex (i tłumaczenie main-debs-pl.tex) wypełniony
danymi kampanii: Setup (platforma + dyscyplina pomiarowa), Throughput
and Latency (tabele z rotated/10 dla 360/720 Hz i rotated/09 dla
1080 Hz), Latency Isolation (kampania clients), Effect of Query-Plan
Depth (fir-contrast). Podsekcje Baselines i Exactness pozostają jako
TODO — brak danych. Oba dokumenty kompilują się czysto (12 stron).

### Decyzja: plan badawczy kampanii baseline (REQUIREMENTS.md)

Do REQUIREMENTS.md dopisany plan dwóch kampanii domykających sekcje
7.5/7.6 artykułu (szczegóły i uzasadnienia w REQUIREMENTS.md, sekcja
„Plan badawczy — kampanie baseline"):

1. **baseline-numpy (priorytet 1)**: potok Pan–Tompkinsa w float64
   (NumPy/SciPy na workerze), dwa tryby raportowane osobno — per-slot
   (odpowiednik E1, z narzutem interpretera) i batch (lfilter);
   dyscyplina środowiska identyczna z kampaniami QRS. Hipoteza: koszt
   semantyki wymiernej pomijalny (callgrind: boost::rational <0,4%),
   różnice zdominuje model wykonania. Ten sam baseline zasila 7.6
   (round-trip interleave/de-interleave vs scipy resample + bitowa
   równość artefaktów między przebiegami).
2. **baseline-flink (priorytet 2, expected fail)**: próba MiniCluster
   na Pi 400 z czterema zastrzeżeniami (4 GB RAM vs JVM, jitter GC/JIT
   przy 360 Hz, brak semantyki slotowej → ryzyko strawmana, nietypowa
   konfiguracja przy izolacji rdzenia); kryterium stopu: 1 dzień
   roboczy, porażka dokumentowana jako wynik negatywny. Opcja
   zapasowa: wariant silnika „engine-double" (przełącznik kompilacji
   double zamiast arytmetyki wymiernej).

### Infrastruktura kampanii baseline-numpy zbudowana i przetestowana dymnie

- `config/pan_tompkins_numpy.py` — wersjonowana implementacja potoku
  (etapy identyczne z rec205-detect.rql, float64; orientacja okna
  przejęta z examples/ecg/ref_float.py, ta sama uwaga o konieczności
  potwierdzenia semantyki AGSE przed publikacją liczb). Tryb `slot`
  emituje CSV zgodny z RDB_BENCH_CSV (analiza tym samym e1_stats.py);
  tryb `batch` — wektoryzowany, R powtórzeń. SCHED_FIFO: własna próba
  sched_setscheduler + okno settle na `chrt -f 50 -p` z runnera
  (własność plików bez zmian — analogia do ścieżki setcap).
- `worker/run_numpy_baseline.sh` — runner wg metodyki kampanii
  (migawki stanu, governor performance z trapem, taskset -c 3 /
  sampler na 0-2, /dev/shm, wersje python/numpy przypięte w
  results.md, commit amend + force-with-lease).
- Smoke test lokalny (x86): tryb slot 3000 próbek @2000 Hz — sonda
  czytelna dla e1_stats.py (E1 mediana 24,4 µs — rząd narzutu
  interpretera widoczny już na x86); batch 650k × 3 — 89 ns/próbkę.
  Zgodność numeryczna slot↔batch: max |Δ| = 8,5e-14 (względnie
  4,9e-16), 0/5000 rozbieżnych decyzji QRS.
- Do wykonania na workerze: pełna kampania (20k @360 Hz slot + batch)
  po decyzji prowadzącego.

### Kampania baseline-numpy wykonana na workerze (branch experiment/20260717)

> **_NOTE:_** Artefakty: [results_20260717/baseline-numpy/](results_20260717/baseline-numpy/)
> (results.md, probe_slot.csv, metrics.csv, state_before/after.md,
> logi obu trybów).

Przygotowanie: worker zastany na branchu 20260716 z **wyzerowanym**
JOURNAL.md w working tree (0 bajtów, niezacommitowane; przyczyna
nieustalona) — przywrócony `git checkout --`, drzewo czyste przed
badaniem.

Droga do poprawnego SCHED_FIFO — dwie usterki runnera, obie wykryte
dzięki polu `scheduler` w META (drukowanym po fazie settle, więc
rozstrzygającym):

1. **Składnia chrt** (commit 716d5b5): `chrt -f 50 -p PID` każe chrt
   *wykonać* „-p" jako polecenie (exit 127) — poprawnie
   `chrt -f -p 50 PID`. Przebieg 1 pobiegł na SCHED_OTHER (zachowany
   w historii brancha, commit 5dd27bb).
2. **Zły pid** (commit bc2dbda): `$!` po `timeout taskset python3 &`
   wskazuje proces `timeout`, nie interpretera; FIFO nadane rodzicowi
   nie przenosi się na już istniejące dziecko — runner raportował
   sukces chrt, a META dalej OTHER (przebieg 3). Dlatego pipeline
   drukuje `PID <n>` przed settle — runner czyta go teraz z logu.

Porównanie OTHER↔FIFO (przebiegi 1 i 4, identyczne parametry):
mediany praktycznie bez zmian (E1 77,9 → 77,0 µs), różnica wyłącznie
w ogonach — E1 max 754,6 → 328,0 µs, wake_lag max 441,2 → 56,1 µs.
Na rdzeniu izolowanym (isolcpus=3) SCHED_FIFO działa jak domknięcie
ogona, nie akcelerator — spójne z obserwacją K6.

**Wyniki (FIFO 50, taskset -c 3, governor performance, isolcpus=3,
20 000 próbek @360 Hz, python 3.12.3 / numpy 1.26.4):**

| Metryka | mediana | p99 | max | budżet 2777,8 µs |
|---|---|---|---|---|
| E1 (slot) | 77,0 µs | 215,8 | 328,0 | max = 11,8% ✓ |
| E2E (slot) | 112,8 µs | 293,7 | 368,6 | max = 13,3% ✓ |
| wake_lag | 23,2 µs | 42,1 | 56,1 | — |

Batch (650k próbek × 5, bez semantyki slotowej): 709 ns/próbkę,
~1,41 mln próbek/s. Sampler: load1 ≈ 0,48, temp ≈ 36,8°C.

Wnioski dla 7.5(i):

- Per-slot float64 (NumPy) mediana E1 = 77 µs wobec 1891 µs silnika
  (kampania clients, study_01, identyczne warunki) — **czynnik ~24,5×**.
  Zgodnie z hipotezą planu badawczego różnicy NIE wyjaśnia arytmetyka
  wymierna (callgrind: boost::rational <0,4%) — to model wykonania:
  wektoryzowane jądra C/BLAS na oknach kontra interpretowany
  14-etapowy plan zapytań z warstwą storage/metaData/IPC per slot.
  Do uczciwego opisu w artykule: NumPy per slot NIE świadczy usług
  silnika (trwałość, wielu klientów, retrakcja, RQL) — mierzy dolną
  granicę kosztu samych obliczeń w float64.
- wake_lag mediana 23,2 µs — praktycznie identyczny z silnikiem
  (22-23 µs w K8/K9/clients): dyscyplina pomiarowa obu badań jest
  porównywalna (ten sam rdzeń, governor, polityka RT); różnice E1/E2E
  są więc atrybutowalne do warstwy obliczeń, nie do środowiska.
- Batch 709 ns/próbkę to ~×109 wobec trybu slot per próbka — skala
  narzutu interpretera CPython + semantyki slotowej; raportowane
  osobno, nieporównywalne wprost (zgodnie z planem badawczym).

### Kampania baseline-flink, krok 1: instalacja + smoke test —
### wynik pozytywny (branch experiment/20260717, commit 5bd7c27)

> **_NOTE:_** Artefakty: [results_20260717/baseline-flink/](results_20260717/baseline-flink/)
> (results.md, smoke_stdout.log, ram_sample.csv, state_before/after.md);
> skrypt: `worker/run_flink_baseline.sh` (commit 3689650).

Plan badawczy (REQUIREMENTS.md, "kampania baseline-flink, priorytet 2")
spisał cztery zastrzeżenia co do wykonalności na Pi 400, z RAM (4 GB
wobec JobManager+TaskManager+heap+metaspace) jako pierwszym. Krok 1 to
sama instalacja + trywialny smoke test, przed napisaniem równoważnego
dataflow Pan–Tompkinsa — sprawdzenie, czy w ogóle jest co dalej robić,
zanim zainwestuje się czas w port DataStream API.

Wykonano: `apt-get install openjdk-17-jdk-headless` (aarch64, kandydat
17.0.19+10); pobranie `flink-2.3.0-bin-scala_2.12.tgz` (604 MB, ~750
KB/s po WiFi — jedno przerwanie transferu na 120 s timeout, dokończone
`curl -C -`), weryfikacja sha512, rozpakowanie. Struktura pakietu
sprawdzona na sprzęcie, nie z pamięci: `conf/config.yaml` (nie
`flink-conf.yaml` — nazewnictwo od Flink 2.x), checkpointing wyłączony
domyślnie (`execution.checkpointing.interval` nieustawiony, wystarczy
nie dotykać).

**Tryb local/MiniCluster w sensie planu ("jeden JVM")**: nie
`bin/start-cluster.sh` (to uruchamia JobManager i TaskManager jako
odrębne procesy demonów — dwa JVM), ale `bin/flink run -t local` —
CLI embeduje MiniCluster we własnym procesie na czas joba i kończy go
po zakończeniu. To dosłownie realizuje "local, jeden JVM" z planu.

Smoke test: wbudowany przykład `examples/streaming/WordCount.jar`,
pamięć ograniczona przez `-D`: `jobmanager.memory.process.size=768m`,
`taskmanager.memory.process.size=1024m` (razem ~1,75 GB wobec 3,7 GiB
RAM, 0 swap).

| Metryka | wartość |
|---|---|
| exit_code | 0 |
| Job Runtime (Flink) | 2936 ms |
| czas całkowity (JVM start + job) | 18,7 s |
| peak RAM systemowy w trakcie | 568-589 MB (dwa przebiegi) |

**Wniosek kroku 1**: instalacja i wykonanie trywialnego joba w trybie
MiniCluster na Pi 400 są wykonywalne w budżecie pamięci znacznie
niższym niż zastrzeżenie planu sugerowało — dla joba tej skali RAM nie
jest blokerem. To NIE anuluje pozostałych trzech zastrzeżeń planu
(jitter GC/JIT przy 360 Hz per-rekord, brak semantyki slotowej,
nietypowa izolacja rdzenia dla całego JVM na rdzeniu 3) — smoke test
nie mierzył per-rekord, tylko wykonywalność. Kryterium stopu kampanii
(budżet 1 dnia roboczego) nie zostało wyczerpane; krok 1 zajął ~10 min
łącznie z pobraniem.

**Decyzja (do potwierdzenia przez prowadzącego)**: krok 2 — port
równoważnego dataflow Pan–Tompkinsa na DataStream API i pomiar
per-rekord — jest zasadny do podjęcia, bo krok 1 nie sfalsyfikował
wykonalności. Pozostałe trzy zastrzeżenia planu (GC/JIT, semantyka
slotowa, izolacja rdzenia) będą rozstrzygane dopiero na tym etapie.

### Kampania baseline-flink, krok 2: równoważny dataflow Pan–Tompkinsa —
### zastrzeżenie #2 planu (jitter JVM) POTWIERDZONE pomiarem (branch
### experiment/20260717, commit finalny 49669ab)

> **_NOTE:_** Artefakty: [results_20260717/baseline-flink/](results_20260717/baseline-flink/)
> (`results_pantompkins.md`, `probe_flink.csv`, `PanTompkinsFlinkJob.java`,
> `metrics.csv`, migawki stanu); skrypty: `worker/run_flink_pantompkins.sh`
> (commit 4dd891d), poprawka bufora `a6a601c`.

**Implementacja** (`config/PanTompkinsFlinkJob.java`): DataStream API,
łańcuch operatorów BandPass→Derivative→SquareMwi→Threshold→ProbeSink
(Tuple8, te same okna 25/5/30/180 i formuła co
`pan_tompkins_numpy.py --mode slot`), źródło z paced `RichSourceFunction`
(absolutne terminy slotów, identyczny wzorzec co numpy/silnik `-t`).
Klasy legacy (`SourceFunction`/`RichSinkFunction` w pakiecie `.legacy`,
`open(OpenContext)`) zweryfikowane na miejscu przez `javap` na
`flink-dist-2.3.0.jar` przed napisaniem kodu — Flink 2.x przeniósł te
API, ale ich nie usunął. Sonda w formacie `RDB_BENCH_CSV` — analiza tym
samym `e1_stats.py`, bez zmian w narzędziu.

**Odkrycie metodyczne po drodze**: `/dev/shm` czyści się między
zamknięciami sesji SSH (znana zagadka `RemoveIPC=yes` z dnia 1,
[K7bis](#kampania-7bis-priorytet-50--fix-4-i-diagnoza-anomalii-32-ms))
— dwa nieudane manualne smoke testy (pliki zniknęły między poleceniami
`ssh`), naprawione przez łączenie przygotowania danych + przebiegu +
odczytu wyników w JEDNEJ sesji. Potwierdza regułę z K7bis: diagnostyka
na tym sprzęcie musi być jednosesyjna.

**Bug #1 znaleziony i naprawiony — `execution.buffer-timeout`**:
domyślna wartość Flinka (100 ms, potwierdzona przez `javap` —
`StreamExecutionEnvironment.setBufferTimeout` istnieje) opóźnia flush
kanału między operatorami do wypełnienia bufora lub upływu timeoutu;
przy 360 Hz (1 rekord / 2,8 ms) bufor nigdy się nie wypełnia, więc
KAŻDY rekord czekał do 100 ms na flush — to nie własność Flinka pod
testem, to przeoczony parametr tuningu. Fix: `env.setBufferTimeout(0)`
(flush natychmiastowy, rekomendacja Flinka dla low-latency). Efekt
(przebiegi diagnostyczne 20k próbek, taskset -c 3, przed/po fixie):
p99,9 wake_lag 80,1 ms → 19,3 ms (**-76%**); mediana wake_lag bez zmian
(~600 µs) — fix usunął jeden składnik ogona, nie floor mediany.

**Diagnoza #2 — pauzy GC (log `-Xlog:gc*`, taskset -c 3, przed fixem
buffera)**: 54 pauzy na 20 000 rekordów / 60 s przebiegu; najdłuższe:
Full GC (Allocation Failure / Metadata GC Threshold) do **93,4 ms**,
Young GC do **57,2 ms**. Bezpośredni dowód przyczyny ogona — nie
hipoteza, zmierzone.

**Diagnoza #3 — samo-rywalizacja `taskset` z wątkami JVM**: `taskset -c 3`
(dyscyplina identyczna z numpy/silnikiem) zamyka na JEDNYM rdzeniu
zarówno wątek pomiarowy, JAK I własne wątki JVM (GC, kompilator JIT) —
inaczej niż w numpy/silniku (jeden wątek, `taskset` daje wyłączność).
Porównanie diagnostyczne (20k próbek, bez fixu buffera): z `taskset`
max wake_lag = 132,8 ms; bez `taskset` (JVM ma 4 rdzenie, wątki GC/JIT
nie rywalizują z wątkiem pomiarowym) max wake_lag = **18,8 ms**
(**-86%**) — ale mediana wake_lag praktycznie identyczna (659 µs vs
649 µs) w obu wariantach. Wniosek: `taskset` na wielowątkowym runtime
pogarsza NAJGORSZY przypadek (rywalizacja o rdzeń podczas GC), nie
dotyka mediany. Decyzja: kampania oficjalna (committed) zachowuje
`taskset -c 3` dla spójności środowiskowej z resztą eksperymentu —
efekt ten jest udokumentowany jako ograniczenie interpretacyjne, nie
naprawiony (naprawa wymagałaby złamania konwencji izolacji rdzenia
wspólnej dla całego eksperymentu).

**Wynik finalny (kampania oficjalna, po fixie buffera, 20 000 próbek
@360 Hz, governor performance, taskset -c 3, BRAK SCHED_FIFO na JVM —
patrz odstępstwo w skrypcie):**

| Metryka | mediana | p99 | p99,9 | max | budżet 2777,8 µs |
|---|---|---|---|---|---|
| E1 (compute) | 4,7 µs | 49,1 | — | 54 714,8 | max = **1969,7%** |
| E2E | 607,7 µs | 1231,3 | 25 470,6 | 55 745,3 | max = **2006,8%** |
| wake_lag | 593,8 µs | 1105,2 | 24 457,2 | 52 981,6 | — |

**Porównanie z numpy/silnikiem (mediany, ten sam sprzęt, @360 Hz):**

| System | E1/compute mediana | wake_lag mediana | ogon max |
|---|---|---|---|
| Flink (krok 2) | 4,7 µs | 593,8 µs | 52 981,6 µs |
| numpy (float64, interpretowany) | 77,0 µs | 23,2 µs | 56,1 µs |
| silnik RetractorDB (K8/clients) | ~1865 µs (E1) | 22-23 µs | ~40 000 µs |

**Wnioski dla 7.5(ii):**

1. **Zastrzeżenie #1 planu (RAM) — obalone** (krok 1): instalacja i
   wykonanie w budżecie pamięci nie stanowią problemu dla tej skali
   joba.
2. **Koszt czystych obliczeń (E1) — Flink WYGRYWA**: po rozgrzewce JIT
   (20k iteracji) 4,7 µs mediana — szybciej niż numpy interpretowany
   (77 µs) i tego samego rzędu co silnik natywny. Kompilacja JIT
   *pomaga* w typowym przypadku, wbrew a priori intuicji planu.
3. **Zastrzeżenie #2 planu (jitter GC/JIT) — POTWIERDZONE pomiarem,
   liczbowo**: mediana wake_lag (594-659 µs, niezależnie od
   `taskset`/bufora) jest **7-28× wyższa** niż numpy (23,2 µs) i
   silnik (22-23 µs) na IDENTYCZNYM sprzęku — floor nieusuwalny naszą
   dyscypliną środowiska, przypisywalny do samego runtime'u JVM
   (ziarnistość `Thread.sleep`/bezpieczniki JIT), nie do GC (obecny
   też w przebiegu z niemal zerowym wpływem pełnych GC). Ogon (max
   53-135 ms wg wariantu) to bezpośredni, zmierzony efekt pauz GC
   (log `-Xlog:gc`) — 9-49× budżet 2,78 ms.
4. **Zastrzeżenie #3 planu (brak semantyki slotowej) — częściowo
   zaadresowane implementacją** (paced source z absolutnymi terminami
   jak numpy/silnik), ale ujawniło pochodny problem infrastrukturalny
   (buffer-timeout) niewidoczny bez tej implementacji — sama próba
   odtworzenia semantyki slotowej w Flinku była pouczająca.
5. **Werdykt całościowy**: Flink na Pi 400 jest zdolny do przetwarzania
   z medianą lepszą niż baseline numpy, ale kategorycznie NIE spełnia
   rygoru czasu rzeczywistego przy 360 Hz — ogon p99,9/max (24,5-55,7 ms,
   9-20× budżet) wynika ze WŁAŚCIWOŚCI RUNTIME'U (GC, ziarnistość
   planisty JVM), nie z modelu dataflow czy naszej dyscypliny
   środowiskowej, która zresztą w jednym wymiarze (`taskset` na
   wielowątkowym procesie) pogarsza sytuację. To dokładnie zastrzeżenia
   #2/#3 z planu badawczego — teraz z liczbami, nie a priori.

**Kryterium stopu**: budżet 1 dnia roboczego NIE wyczerpany (kroki 1+2
łącznie: kilka przebiegów ~60 s + diagnostyka, rząd wielkości godzin,
nie dnia).

**Decyzja prowadzącego (zamknięcie kampanii)**: kampania baseline-flink
uznana za zamkniętą — materiał dla 7.5(ii) wystarczający, opcje
dalszej optymalizacji (ZGC/Shenandoah, „engine-double") pozostają
niewykorzystane, poza zakresem. Komentarz prowadzącego, wart
odnotowania jako podsumowanie kampanii: **zaskoczenie** — dało się w
ogóle zainstalować i uruchomić na Pi 400 w budżecie pamięci (zastrzeżenie
#1 planu, RAM, było główną a priori obawą); **brak zaskoczenia** — Java/JVM
nie nadaje się do rygoru czasu rzeczywistego (zastrzeżenia #2/#3 planu,
teraz potwierdzone pomiarem, nie a priori). Worker po zamknięciu:
brak procesów java/flink, governor przywrócony do `ondemand`.

### Sekcja 7.5 (Baselines) artykułu wypełniona wynikami obu kampanii

Na polecenie prowadzącego wyniki baseline-numpy i baseline-flink
(z [results_20260717/baseline-numpy/](results_20260717/baseline-numpy/) i
[results_20260717/baseline-flink/](results_20260717/baseline-flink/); katalog `results/`
zostanie na koniec dnia przeniesiony do `results_20260717/`) wpisane do
main-debs.tex i main-debs-pl.tex:

- **7.5 Baselines**: nowa tabela `tab:eval-baselines` (RetractorDB /
  NumPy per-slot / Flink @360 Hz: mediany compute, wake-lag, E2E oraz
  max E2E) + trzy akapity: (i) numpy — czynnik ~25× przypisany modelowi
  wykonania, nie arytmetyce wymiernej (boost::rational <0,4%
  instrukcji), identyczny wake_lag 23 µs jako dowód porównywalności
  środowisk, batch 709 ns/próbkę raportowany osobno; (ii) Flink —
  mediana compute 4,7 µs najniższa z trzech (z uczciwą uwagą o jednym
  przejściu kolejki źródło→operator w sondzie), ale floor wake_lag
  594 µs (~26×) i ogon 25,5/55,7 ms (9–20× budżetu) przypisany logami
  GC; odnotowane oba dostosowania (buffer-timeout=0, brak SCHED_FIFO
  dla JVM) i efekt taskset na wielowątkowym runtime; (iii) werdykt —
  utrzymanie osi slotów wymaga procesu podatnego na dyscyplinę RT,
  którą oba procesy natywne spełniają, a JVM nie.
- Zaktualizowane miejsca „baseline w toku": abstrakt (nowe zdanie
  zamykające z werdyktem baseline'ów), wstęp, otwarcie sekcji 7
  (jedyną zaległą częścią pozostaje 7.6 exactness/replay), akapit
  „Performance evaluation" w Discussion (twierdzenia porównawcze
  ograniczone do modeli wykonania na tej płytce). Komentarze TODO-EVAL
  zawężone do części exactness.
- Kompilacja czysta: EN 12 stron, PL 13 stron, zero ostrzeżeń LaTeX.
- Otwarte: sekcja 7.6 (Exactness and Replay Stability) — nadal TODO,
  czeka na kampanię exactness/replay.

### Przygotowanie kampanii 7.6 — testy dymne lokalne (x86, Debug):
### determinizm artefaktów POTWIERDZONY, round-trip rozplotu ZABLOKOWANY
### błędem silnika

Dwa testy dymne przed budową infrastruktury kampanii (scratchpad,
poza repo; binarki z master:305506f).

**7.6(a) — determinizm artefaktów: wynik pozytywny z jednym
udokumentowanym wyjątkiem.** Wariant rec205-detect.rql z usuniętym
VOLATILE (17 strumieni zapisywanych), 2 przebiegi × 2000 cykli,
porównanie sha256 wszystkich artefaktów: **wszystkie pliki danych
i `.desc` identyczne co do bitu**; różnią się wyłącznie `.meta` —
i to dokładnie w 8 bajtach nagłówka: `metaIndexStore` zapisuje tam
czas utworzenia (`system_clock::now()`, metaIndexStore.cc:36,
nanosekundy epoki). Po odcięciu 8-bajtowego znacznika wszystkie
`.meta` również identyczne. Definicja porównania dla kampanii:
równość bitowa modulo znacznik czasu utworzenia w nagłówku `.meta`
(do jawnego opisania w artykule).

**7.6(b) — tożsamość okrężna: dwa defekty silnika znalezione przed
kampanią.** Ustalenia po drodze: (1) odwołanie do pola przeplotu
wymaga self-referencji (`SELECT c[0] STREAM c FROM a#b`, jak w teście
it_operations); (2) argument `&`/`%` to Δ strumienia usuwanego —
Δ_out = (Δ_c·x)/|Δ_c−x| (compiler.cpp, STREAM_DEHASH_*); (3) rozplot
działa tylko na źródle DEKLAROWANYM (bufor cykliczny) — naturalny
schemat dwufazowy: program 1 zapisuje przeplot jako artefakt,
program 2 DECLARE z pliku artefaktu i rozplata (zgodne z
„transmisyjnością artefaktów" z artykułu).

Test kontrolowany (sa=1..N @1/16, sb=1001.. @1/8, c=sa#sb @1/24,
wzorzec AAB): **przeplot przy różnych Δ dokładny** (c = 1,2,1001,
3,4,1002,… — bez strat, duplikatów, przestawień). Defekty:

1. **Rozplot szybszej składowej: błąd fazy o 1 pozycję.** Argument
   1/16 odzyskuje sb DOKŁADNIE (1001,1002,…,1011 — cor:exact
   potwierdzone dla wolniejszej składowej). Argument 1/8 zamiast
   sa=(1,2,3,4,…) daje (2,1001,4,1002,6,1003,…) — podciąg o dobrej
   gęstości 2/3, ale przesunięty o 1 (indeksy 1,2,4,5,… zamiast
   0,1,3,4,…): wynik zawiera krotki OBU strumieni — wprost narusza
   cor:exact. Podejrzenie: rozjazd floor/ceil w formułach
   Div/Mod (SOperations.hpp: Div = i+ceil((i+1)·dA/dB); poprawne
   pozycje szybszej składowej to i+floor(i·dA/dB)).
2. **`&` i `%` dają identyczne wyjścia** przy tym samym argumencie
   (testowane 4 kombinacje × 2 argumenty) — a realizują formalnie
   różne operatory (Θ vs ~Θ).
3. **Przeplot równych Δ (1/360 # 1/360): składowa B w całości
   nullowa** (c = a0,0,a1,0,… zamiast a0,b0,a1,b1,…) — degeneracja
   w torze STREAM_HASH przy Δa=Δb (przypadek „perfect shuffle",
   formalnie objęty def:interleave).

**Wniosek: kampania 7.6(b) na workerze jest zablokowana do czasu
naprawy rozplotu w silniku** (kandydat: fix #6, SOperations.hpp /
tor STREAM_DEHASH). 7.6(a) jest gotowe do wykonania. Decyzja o
naprawie rdzenia — do prowadzącego.

### Fix #6 — przeplot/rozplot zgodny z definicjami formalnymi
### (decyzja prowadzącego: naprawić i pokryć testem; wskazówka
### prowadzącego: stary test soperations mógł być błędny — potwierdzona)

Diagnoza pełna (trzy współdziałające defekty toru `#`/`&`/`%`):

1. **`Hash` (SOperations.hpp) używał Δ_c zamiast z** = Δb/(Δa+Δb)
   w formułach retPos — maskowane w starym teście jednostkowym przez
   Δa=1 (wtedy Δ_c == z liczbowo). Wskazówka prowadzącego trafna:
   tabela oczekiwań `divmod_operations` była drukowana z implementacji
   (`std::cout` w pętli testu), choć akurat Div/Mod zgadzały się
   z definicjami Θ/~Θ — błędne były miejsca WYWOŁAŃ.
2. **dataModel wołał Hash 1-bazowo** (`count+1`) i traktował wynik
   (indeks postępujący składowej) jak offset WSTECZNY (`revRead`),
   który dla źródeł deklarowanych był w dodatku ignorowany
   (`getPayload` pomija `revRead` dla declared) — stąd „działające"
   przeplatanie deklaracji (przypadkowa poprawność przez kadencję
   prefetch) i zerowe wyniki dla źródeł pochodnych.
3. **DEHASH dostawał `lengthOfSrc`** (licznik rekordów ŹRÓDŁA) zamiast
   0-bazowego indeksu rekordu WYJŚCIOWEGO, przez co offsety zawsze
   wypadały poza zakres → całe wyjście null.

Naprawa (6 plików): `Hash` na z (wzór z def:interleave); nowy
`dataModel::fetchForward` — konwersja indeksu postępującego na offset
wsteczny względem bieżącego licznika źródła (odporna na kadencję
prefetch), historia deklaracji czytana z bufora cyklicznego **bez
mutacji** bieżącego payloadu (nowe akcesory `storage::history/
historySize`), rekord all-null przy niedostępności; wywołania HASH
i DEHASH 0-bazowo; pojemność historii źródeł `#`/`&`/`%` podnoszona
w kompilatorze do stałej `kJunctionHistory=4` (offset wsteczny jest
stały w rekordach niezależnie od proporcji Δ — inaczej niż w AGSE,
gdzie lookback rośnie z oknem; pytanie prowadzącego o wyliczanie
zamiast stałej rozstrzygnięte wyprowadzeniem w komentarzu).

**Ustalenie semantyczne**: Θ (`&`) jest w silniku z natury
nieprzyczynowa — a_n = c_{n+⌈(n+1)Δa/Δb⌉} powstaje PO slocie n —
realizacja przyczynowa opóźnia Θ o jeden slot (rekord 0 = all-null,
poprawnie oznaczony w `.meta`; potem dokładnie a_0, a_1, …). ~Θ (`%`)
jest dokładna od rekordu 0.

Weryfikacja (wzorce liczone z definicji, nie z implementacji):
- jednostkowo: `hash_operations_nonunit_delta` (wyłapuje stary błąd
  z↔Δ_c), `hash_operations_equal_delta` (perfect shuffle),
  `divmod_inverts_hash` (własność cor:exact: Div/Mod trafiają
  w gałąź A/B Hash z tym samym indeksem; 4 pary delt × 200 pozycji);
- integracyjnie: nowy `it_deinterleave_roundtrip` (1/16#1/8 → `&`/`%`,
  porównanie bitowe c/a2/b2 z wzorcami z definicji);
- scenariusze ręczne: dwufazowy round-trip EKG (równe Δ, DECLARE
  z artefaktów): c = b0,a0,b1,a1,…; **a2[1:] == a i b2 == b co do
  bitu**; jednoprogramowy nierówny (deklaracje): dokładny.
- testy: **153/153** (152 stare + nowy; zmiana fazy przeplotu na
  zgodną z definicją nie psuje żadnego istniejącego testu).

**Znane ograniczenie (poza zakresem fixu #6, kandydat na fix #7):**
przeplot źródeł POCHODNYCH (SELECT→`#`) emituje null tam, gdzie
element składowej nie istnieje jeszcze w chwili slotu c — planista
daje c sloty zanim producenci wykonają swoje (kolejność wykonania
sortowana po rInterval rosnąco + nadmiarowe wczesne sloty c);
kampania 7.6 nie jest tym dotknięta (schemat dwufazowy przez
artefakty — zgodny z „transmisyjnością artefaktów" z artykułu).
Odnotowano też pokrewny trop: `STREAM_TIMEMOVE` (operator `>`)
na źródle deklarowanym ignoruje offset (ta sama ścieżka getPayload)
— do osobnego zbadania.

Fix #6 przeniesiony na master cherry-pickiem (e87b9ab; konflikt tylko
w JOURNAL.md — master nie miał wpisów dnia 2 — rozwiązany wersją
z brancha eksperymentu); pełny zestaw testów na masterze: 153/153.

---

## 2026-07-18 — dzień 3: kampania exactness (7.6)

### Rano — domknięcie dnia 2 i przygotowanie kampanii

- Rotacja wyników dnia 2 (zapowiedziana przez prowadzącego):
  `results/baseline-{numpy,flink}` → [results_20260717/](results_20260717/)
  (commit ec9c62d na branchu experiment/20260717).
- Dzień 2 scalony do master squashem (fd221d6) — wzorzec z dnia 1;
  master zawiera fix #6, wyniki baseline w results_20260717/ i dziennik.
- Infrastruktura kampanii exactness na master (27880d2), zgodnie
  z planem badawczym „Część dla 7.6" (REQUIREMENTS.md): trzy badania —
  A replay (2× potok QRS z zapisem WSZYSTKICH 17 strumieni, porównanie
  bitowe artefaktów; `.meta` po odcięciu 8-bajtowego znacznika czasu
  utworzenia), B round-trip przeplotu/rozplotu na kanałach EKG
  (cor:exact, schemat dwufazowy przez artefakty — rozplot wymaga
  źródła deklarowanego), C referencja float
  ([config/resample_roundtrip.py](config/resample_roundtrip.py):
  scipy `resample_poly` i `resample`, round-trip 360→720→360 Hz,
  normy błędu pełna/wnętrze w funkcji N z siatki 1250…650000).
- Decyzja metodyczna: przebiegi unpaced (bez `-t`) — mierzoną
  własnością jest równość bitowa, nie czas; dyscyplina środowiska
  (governor performance, taskset -c 3, /dev/shm) utrzymana dla
  spójności z kampaniami dni 1–2.
- Branch experiment/20260718 utworzony z master (zgodnie z pkt 23:
  master zsynchronizowany z origin na obu maszynach); worker
  zsynchronizowany, kampania uruchomiona przez
  [worker/run_exactness.sh](worker/run_exactness.sh) (rebuild
  Release+probe z fixem #6 + 3 badania + commit konwencją amend).
- Smoke test referencji float lokalnie (x86): `resample_poly` ma błąd
  strukturalny (wnętrze max ~0,5, RMS ~0,11 jednostki ADC; brzegi do
  ~184), `resample` (FFT) schodzi do błędu zaokrągleń float64
  (~1e-13), który rośnie z N — obie metody NIEZEROWE wobec dokładnego
  zera silnika. Liczby do artykułu wyłącznie z workera (przypięte
  wersje: python 3.12.3, numpy 1.26.4, scipy 1.11.4).
- Dodatkowo przygotowany przebieg lokalny x86 (te same parametry:
  rec205, 20 000 próbek, exactness-replay.rql) — hashe artefaktów do
  testu równości MIĘDZYPLATFORMOWEJ (x86-64 vs aarch64) po zakończeniu
  kampanii na workerze.

### Przebieg 1 kampanii: A=OK, B=fałszywy FAIL (błąd weryfikatora,
### nie silnika) — incydent metodyczny odnotowany zgodnie z regułą

Badanie A (replay) na workerze: **OK** — wszystkie pliki danych,
`.desc` i `.shadow` obu przebiegów identyczne; `.meta` identyczne po
odcięciu 8-bajtowego znacznika czasu utworzenia (klasyfikacja
IDENT-PO-TIMESTAMP w replay_compare.txt).

Badanie B zgłosiło FAIL, ale sprawdzenia merytoryczne PRZESZŁY
(a2[1:]==a i b2==b co do bitu na danych workera); pękło pomocnicze
sprawdzenie wzorca c po stronie nieparzystej. Diagnoza reprodukcją
lokalną w tej samej skali: **zero niezgodności w danych** — błąd był
w mojej formule weryfikatora (w 39995 rekordach c pozycji
nieparzystych jest 19997, porównanie brało `len(c)-n`=19998 elementów;
`np.array_equal` na różnych długościach zwraca False). Poprawka
runnera (porównania na wspólnych prefiksach, commit ddfbd34 na master,
cherry-pick 526d510 na branch), kampania powtórzona w całości.

### Równość międzyplatformowa artefaktów (x86-64 ↔ aarch64) — BONUS

Porównanie hashów badania A z workera (aarch64, Release+probe,
GCC Ubuntu 24.04) z przebiegiem lokalnym x86-64 (Debug, GCC 15.2),
identyczne parametry: **51/51 artefaktów identycznych co do bitu**
(17 strumieni: dane + .desc + .shadow oraz .desc deklaracji).
Uwaga prowadzącego (zasadna a priori): różnice mogłyby wynikać
z odmiennego wyrównania niepakowanych struktur x86 vs ARM — nie
manifestują się, bo artefakty nie są zrzutami struktur C++: layout
rekordu definiuje Descriptor (jawne rozmiary pól, getSizeInBytes),
a obie platformy są little-endian LP64. Ustalenie wzmacnia tezę
o transmisyjności artefaktów między maszynami (sekcja System
artykułu) i jest warte zdania w 7.6.

### Przebieg 2 kampanii: A=OK, B=OK — kampania exactness ZAMKNIĘTA

> **_NOTE:_** Artefakty: [results_20260718/exactness/](results_20260718/exactness/)
> (results.md, replay_hashes_run1/2.txt, replay_compare.txt,
> roundtrip_compare.txt, float_roundtrip.csv, metrics.csv, migawki
> stanu); commit 4f312ef na branchu experiment/20260718.

Wyniki (worker: aarch64 Release GCC 14.2.0, binarka 526d510;
python 3.12.3, numpy 1.26.4, scipy 1.11.4 — wersje przypięte):

**A — stabilność odtwarzania**: dwa przebiegi potoku QRS (20 000
próbek, 17 strumieni zapisywanych) → 67 artefaktów: 51 (dane, .desc,
.shadow) **identycznych co do bitu**, 16 `.meta` identycznych po
odcięciu 8-bajtowego znacznika czasu utworzenia w nagłówku — jedyne
niedeterministyczne bajty, jakie silnik zapisuje. Plus równość
międzyplatformowa (wpis wyżej): te same artefakty na x86-64 i aarch64.

**B — tożsamość okrężna (cor:exact)**: c = a#b (39 995 rekordów
@720 Hz) odtwarza wzorzec definicji przeplotu dokładnie po obu
stronach (parzyste==b: 19 998, nieparzyste==a: 19 997); rozplot:
**a2[1:] == a i b2 == b co do bitu** (Θ z jednoslotowym opóźnieniem,
rekord 0 all-null oznaczony w .meta; ~Θ dokładna od rekordu 0).

**C — kontrast float** (round-trip 360→720→360 Hz, MLII, float64):
- `resample_poly` (FIR polifazowy): błąd WNĘTRZA strukturalny
  i niezależny od długości (RMS ≈ 0,111 jedn. ADC, max 0,39–0,64);
  brzegi do 184,4 jedn. (pełny max stały, pełny RMS maleje z N tylko
  przez rozcieńczenie efektu brzegowego: 6,21 → 0,30).
- `resample` (FFT): błąd na poziomie zaokrągleń float64, ale NIEZEROWY
  i ROSNĄCY z długością nagrania: max |err| 5,68e-13 (N=1250) →
  1,36e-12 (N=650 000); RMS ~2e-13 ≈ względnie ~2e-16.
- Silnik: błąd ≡ 0 potwierdzony jako równość BITOWA, nie jako małe
  residuum numeryczne.

Bilans kampanii: wszystkie trzy badania rozstrzygnięte za pierwszym
kompletnym przebiegiem (drugi przebieg = powtórka po fałszywym FAIL
weryfikatora); dodatkowy wynik ponadplanowy — równość
międzyplatformowa artefaktów. Sekcja 7.6 artykułu może zostać
wypełniona; po niej w Performance Evaluation nie zostaje żaden TODO.

### Sekcja 7.6 artykułu wypełniona — Performance Evaluation kompletna

Rozdział 7 main-debs.tex (i main-debs-pl.tex) domknięty wynikami
kampanii exactness: 7.6 opisuje replay bit-for-bit (modulo 8-bajtowy
znacznik czasu `.meta`) wraz z równością międzyplatformową
x86-64↔aarch64, tożsamość okrężną cor:exact potwierdzoną bitowo
(z uczciwą notą o jednoslotowym opóźnieniu Θ i zdaniem o dokładności
jako wyroczni testowej — złapany fix #6), oraz kontrast float
(resample_poly: strukturalne ~0,11 RMS; resample FFT: błąd rosnący
z N). Zaktualizowane abstrakt, wstęp, otwarcie sekcji 7 i Limitations;
usunięte wszystkie komentarze TODO-EVAL. Kompilacja czysta:
EN 13 stron, PL 14 stron. Commit 9148864 w repo paper-arXiv.

---

## 2026-07-18 — ZAMKNIĘCIE PROCESU EKSPERYMENTÓW (decyzja prowadzącego)

Trzydniowy proces eksperymentów zakończony. Wszystkie kampanie
zaplanowane w REQUIREMENTS.md wykonane, sekcja Performance Evaluation
artykułu kompletna (7.1–7.6, zero TODO). Wyniki zrotowane do katalogów
dziennych, branche eksperymentów squashowane do master i usunięte.

### Skorowidz pomiarów (stan końcowy na master)

| Dzień | Kampanie | Artefakty |
|---|---|---|
| 1 (2026-07-16) | rate K1–K9 (baseline → fix #1–#5 → isolcpus) | [results_20260716/rate/rotated/01–10](results_20260716/rate/rotated/) |
| 1 | clients (H-izolacja, 1/2/3 klientów) | [results_20260716/clients/](results_20260716/clients/) |
| 1 | fir-contrast (głębokość planu, 1–3 kHz) | [results_20260716/fir-contrast/](results_20260716/fir-contrast/) |
| 2 (2026-07-17) | baseline-numpy (7.5 i) | [results_20260717/baseline-numpy/](results_20260717/baseline-numpy/) |
| 2 | baseline-flink (7.5 ii, kroki 1+2) | [results_20260717/baseline-flink/](results_20260717/baseline-flink/) |
| 3 (2026-07-18) | exactness (7.6: replay + round-trip + float) | [results_20260718/exactness/](results_20260718/exactness/) |

Poprawki silnika wyniesione z procesu (wszystkie na master, każda
z drogą profil→hipoteza→pomiar→dziennik): #1 AGSE cache, #2 Descriptor
const-cache, #3 metaData write-through, #4 leniwe formatowanie,
#5 cache kolejek IPC, #6 przeplot/rozplot zgodny z definicjami
formalnymi (153/153 testów). Otwarte kandydatury na przyszłość:
fix #7 (przeplot źródeł pochodnych — planista), STREAM_TIMEMOVE na
źródle deklarowanym, śledztwo zdarzenia platformowego ~40 ms,
eksperyment na SSD (pkt 18).

### Korekta interpretacji ogona ~40 ms w artykule (po zamknięciu pomiarów)

Przegląd rzetelnościowy main-debs.tex wykazał, że artykuł przypisywał
ogon p99,9 (~40 ms) platformie (firmware/SoC) zbyt stanowczo: śledztwo
jest formalnie otwarte (patrz wyżej), a baseline float64 — taktowany
identycznie na tym samym izolowanym rdzeniu — nie zaobserwował ani
jednego takiego zdarzenia w 20 000 slotów (max E2E 368,6 µs), czego
nie da się pogodzić z czysto platformową atrybucją bez dodatkowych
badań. Symetrycznie: zdanie abstraktu o Flinku ("misses the slot
regime by an order of magnitude in its latency tail") było podatne na
kontrę własną Tabelą 5 (ogon silnika 38/49 ms jest tego samego rzędu
co 25,5/55,7 ms Flinka; p99 Flinka mieści się w budżecie).

Poprawki (main-debs.tex + main-debs-pl.tex, EN 13 str. / PL 14 str.,
bibliografia EN nadal od s. 12, kompilacja czysta):

- abstrakt: różnicą wobec Flinka jest teraz próg pobudki ~26× i ogon
  GC pochodzący z samego środowiska uruchomieniowego, nie "rząd
  wielkości w ogonie";
- 7.2: atrybucja ogona przeformułowana na hipotezę (kandydat:
  platforma; kontrargument: czysty przebieg float64; nie można
  wykluczyć interakcji z torem emisji) — przyczyna jawnie
  "nierozstrzygnięta";
- 7.3: "zdarzenie platformowe" → "zdarzenie ogonowe z 7.2";
  ograniczenie "na poziomie platformy" → "badanej konfiguracji, do
  czasu rozstrzygnięcia przyczyny";
- 7.5 (Flink): dopisane uczciwe zdanie, że ogony obu systemów są tego
  samego rzędu, a różnica leży w pochodzeniu (GC potwierdzone logami
  vs zdarzenia o nieustalonej przyczynie) i w progu pobudki;
- Limitations: "platform events" → zdarzenia o nierozstrzygniętej
  przyczynie z odsyłaczem do 7.2.

Wniosek metodyczny: fix #7-adjacent — śledztwo zdarzenia ~40 ms (pkt
"otwarte kandydatury" wyżej) zyskuje na priorytecie, bo od jego wyniku
zależy siła argumentacji porównawczej artykułu.

Stan maszyn na zamknięciu: worker (pi400) — governor przywrócony,
brak procesów pomiarowych, repo do przełączenia na master; nadzorca —
master zsynchronizowany z origin.

---

## 2026-07-18 — Śledztwo zdarzeń ~40 ms: plan i start Fazy 0

Otwarcie odrębnego procesu badawczego (poza zamkniętym eksperymentem
wydajnościowym): ustalenie pierwotnej przyczyny rzadkich zdarzeń
30–50 ms w wake_lag (ogon p99,9 silnika; wpisy z dni 1–2 oraz korekta
interpretacji w artykule wyżej). Od wyniku zależy siła argumentacji
porównawczej artykułu (7.2/7.5).

### Co już wiemy (przesłanki wyjściowe)

- zdarzenia trafiają w fazę uśpienia (wake_lag), maksimum compute czyste;
- przetrwały isolcpus/nohz_full/rcu_nocbs — to nie zwykły szum planisty;
- baseline NumPy (identyczne taktowanie, ten sam rdzeń, sampler metryk
  aktywny) — ZERO zdarzeń w 20 000 slotów (max E2E 368,6 µs);
- p99,9 skaluje się ~liniowo z liczbą klientów (tor emisji);
- odstępy między zdarzeniami 0,9–2,6 s (dzień 1).

Hipotezy: H1 platforma system-wide (firmware/SoC/mmc/magistrala);
H2 aktywność własna silnika (IPC boost::interprocess → mmap/IPI TLB
shootdown, zapisy tmpfs → kworker per-cpu, logi); H3 interakcja
(np. sampler/WiFi × silnik). Przesłanka NumPy mocno osłabia czyste H1.

### Plan (fazy)

- **Faza 0 — dyskryminacja bez zmian w silniku (ten wpis):**
  S1 pacer-solo — proces-cień (taktowana pętla bez pracy, FIFO 50) SAM
  na rdzeniu 3, 200 000 slotów (~9 min): czy platforma bez silnika
  generuje zdarzenia ≥ 5 ms? S2 engine-shadow — silnik (konfiguracja
  kampanii rate, 1 klient) na rdzeniu 3 + cień na rdzeniu 2, 5×20 000
  próbek: czy zdarzenia silnika widzi równocześnie niezależny proces na
  sąsiednim rdzeniu? Wokół przebiegów: snapshoty /proc/interrupts i
  /proc/softirqs (delta IPI/CAL = ślad TLB shootdown), dmesg.
  Kryteria werdyktu: cień widzi zdarzenia ≈ jak silnik → H1; cień
  czysty przy zdarzeniach silnika → H2 (z zastrzeżeniem stalli
  per-rdzeń — rozstrzyga Faza 2). Koincydencja czasowa w Fazie 0 tylko
  zgrubna (kotwica silnika znana z widełek launch/exit — sonda nie
  zapisuje czasu bezwzględnego, a silnika nie zmieniamy).
- **Faza 1 — ablacja silnika (jedna zmienna na raz):** 0 klientów →
  wszystkie strumienie VOLATILE → logi/CSV poza SD → WiFi off (SSH po
  eth) → bez samplera. Zniknięcie zdarzeń po przełączniku = trigger.
- **Faza 2 — instrumentacja jądra:** tracer osnoise/hwlat; trace-cmd
  z wyzwalanym snapshotem (sonda przy wake_lag > 5 ms → trace_marker);
  liczniki IPI/CAL; audyt mlockall/majflt; wtedy też dokładna
  koincydencja czasowa.
- **Faza 3 — domknięcie:** poprawka silnika (jeśli H2; oczekiwany
  spadek p99,9 o ~2 rzędy) albo udokumentowana atrybucja platformowa
  (jeśli H1) → aktualizacja main-debs.tex 7.2/7.5 z dowodem zamiast
  hipotezy.

### Decyzje procesowe (odstępstwo od reguł eksperymentu wydajnościowego)

- Branch **experiment/40ms** ZAWIERA skrypty badawcze (potencjalnie
  ślepa uliczka — dlatego nie na masterze, w odróżnieniu od
  poprzednich eksperymentów, gdzie branch niósł wyłącznie wyniki).
  REQUIREMENTS.md pkt 23 poluzowany datowanym wyjątkiem; worker buduje
  binarki Z BRANCHA eksperymentu, nie z mastera.
- Jeśli przyczyna zostanie jednoznacznie ustalona — branch będzie
  squashowany i dołączony do mastera; jeśli śledztwo utknie — zostaje
  niezmergowany jako udokumentowana ślepa uliczka.

### Infrastruktura Fazy 0 (dodana na branchu)

- `config/shadow_pacer.py` — proces-cień: absolutne deadline'y na
  CLOCK_MONOTONIC, mechanizm taktowania identyczny z
  pan_tompkins_numpy.py (porównywalność z opublikowanym baselinem),
  protokół "PID + settle + sudo chrt -f -p 50" jak w
  run_numpy_baseline.sh, SIGTERM = partial dump.
- `config/analyze_40ms.py` — percentyle, zdarzenia ≥ progu, odstępy,
  zgrubna koincydencja (z jawnie raportowaną tolerancją), werdykt.
- `worker/run_40ms_phase0.sh` — badania S1/S2 w konwencji
  run_study.sh (governor, snapshoty, /dev/shm, commit amend + push);
  dodatkowo snapshoty IRQ i dmesg.
- `start_40ms_phase0.sh` — nadzorca: checkout brancha na workerze,
  build z brancha (−DRDB_BENCH_PROBE=ON), S1 → reboot → S2.

Start Fazy 0: uruchomiono `start_40ms_phase0.sh` (S1 200k slotów,
S2 5×20k, build z brancha). Wyniki trafią do
`results/40ms/study_{pacer-solo,engine-shadow}/` na branchu; analiza
i werdykt — kolejny wpis.

### Wyniki Fazy 0 — werdykt: TO NIE PLATFORMA; zdarzenia są
### deterministyczne (start + podpięcie klienta), stan ustalony CZYSTY

> **_NOTE:_** Artefakty: branch `experiment/40ms`,
> `results/40ms/study_pacer-solo/` i `study_engine-shadow/`
> (results.md, e1_probe.csv per powtórzenie, shadow.csv.gz,
> irq_before/after, dmesg_tail). Kampania: build z brancha,
> S1 → reboot → S2 5×20k, zakończona 12:28.

**S1 pacer-solo** (cień SAM na rdzeniu 3, FIFO 50, 200 000 slotów
≈ 9,3 min): mediana wake_lag 22,5 µs (identyczna jak u silnika!),
**max 57 µs, ZERO zdarzeń ≥ 5 ms**. Platforma z izolowanym rdzeniem
nie generuje sama z siebie niczego — replikacja czystego NumPy na
10× dłuższym przebiegu. H1 (czysta platforma) obalona w praktyce.

**S2 engine-shadow** (silnik rdzeń 3 + cień rdzeń 2, 5×19 999
slotów): silnik 57–83 „zdarzeń" ≥ 5 ms na przebieg (max 39,1–47,5 ms
— znajoma sygnatura ~40 ms odtworzona), **cień równocześnie: ZERO
zdarzeń, max 0,088–0,125 ms** we wszystkich pięciu powtórzeniach.
Zdarzenia NIE są system-wide.

**Analiza klastrowa (przełom):** zdarzenia nie są rozproszone — w
każdym powtórzeniu układają się w DOKŁADNIE dwie serie kolejnych
slotów o liniowo malejącym opóźnieniu (sygnatura pojedynczego stalla
i doganiania osi czasu przez absolutne deadline'y):

| powt. | seria startowa | seria druga | szczyt drugiej |
|---|---|---|---|
| 1 (po reboocie) | sloty 0–36, szczyt 47,5 ms | sloty 679–724, t=1,89 s | 39,9 ms |
| 2–5 | sloty 0–11, szczyt 19,5 ms | sloty ~709–754, t=1,97 s | 39,1–39,5 ms |

Druga seria występuje ZAWSZE w t≈2,0 s od startu pętli — dokładnie w
momencie `sleep 2` skryptu, po którym podpina się klient xqry.
Długość serii ≈ szczyt/(budżet − compute) ≈ 40/(2,78−1,9)… zgadza się
z obserwowanymi 45–47 slotami. Po slocie 800 (stan ustalony) max
wake_lag w CAŁYM przebiegu to **0,50–0,69 ms** — żadnych zdarzeń
przez pozostałe ~53 s, w pięciu powtórzeniach.

**Werdykt:** pierwotna przyczyna to nie „firmware/SoC housekeeping".
Zdarzenia ~40 ms są deterministycznie związane z (a) inicjalizacją
pętli (kotwica czasu ustawiana PRZED rtActivate/mlockall/otwarciem
sondy — executorsm.cpp:617 vs 620–643; transjent startowy, po
reboocie większy: zimny cache) oraz (b) **podpięciem klienta xqry**
(tor komunikacyjny silnika). To wyjaśnia jednym mechanizmem:
przetrwanie isolcpus (zdarzenie wewnątrzprocesowe), trafianie w fazę
snu przy czystym compute, skalowanie p99,9 ≈ N×40 ms z liczbą
klientów (N podpięć), czystość NumPy/cienia (brak IPC) oraz p99,9
kampanii (serie 45+ slotów > 0,1% z 20k slotów → percentyl ląduje w
serii).

**Hipoteza mechanizmu (do potwierdzenia w Fazie 2):** rtActivate robi
`sched_setscheduler(0,…)` = FIFO 50 TYLKO dla wątku przetwarzającego;
wątek komunikacyjny (bt, executorsm.cpp:544) zostaje na SCHED_OTHER i
dzieli rdzeń 3 (taskset przypina cały proces). Przy podpięciu klienta
wykonuje on tworzenie/mapowanie segmentu boost::interprocess, a
`mlockall(MCL_CURRENT|MCL_FUTURE)` wymusza natychmiastową populację
stron nowego mapowania — kandydat na ~40 ms blokady (mmap_lock /
inwersja na blokadzie procesu), której budzący się wątek FIFO nie
może ominąć. Ftrace/osnoise w Fazie 2 ma pokazać, co dokładnie
wykonuje rdzeń 3 w oknie stalla.

**Korekta wcześniejszej interpretacji (dzień 1):** piki „co 0,9–2,6 s"
sprzed izolacji to była INNA populacja (housekeeping, usunięty przez
isolcpus). To, co „przetrwało izolację", to serie start+attach —
pomylone wtedy z tamtą populacją, bo patrzyliśmy na percentyle, nie
na rozkład zdarzeń w czasie.

**Konsekwencje dla artykułu (po Fazie 2):** narracja 7.2/7.3 do
wzmocnienia: ogon p99,9 nie jest losowym szumem platformy, lecz
deterministycznym kosztem zdarzeń łączeniowych (start, attach);
stan ustalony trzyma <0,7 ms przez 5×53 s. To lepsza i mocniejsza
teza niż obecna hipoteza platformowa — ale wymaga potwierdzenia
mechanizmu i najlepiej poprawki (fix #7-adjacent: FIFO/affinity dla
wątku komunikacyjnego albo prealokacja segmentów IPC + przesunięcie
kotwicy za inicjalizację).

**Następne kroki:** Faza 2 skrócona (ablacja Fazy 1 zbędna — trigger
już zlokalizowany): (1) ftrace/osnoise wokół okna attach; (2) przegląd
ścieżki attach w komunikacji IPC (rozmiar segmentu, moment mmap);
(3) kandydat poprawki i powtórka S2 z oczekiwanym zniknięciem drugiej
serii; (4) osobno: przesunięcie kotwicy pętli za rtActivate (usuwa
transjent startowy z metryk). Wtedy decyzja o squash do mastera
(REQUIREMENTS pkt 23, wyjątek).

## 2026-07-18 — Faza 2: mechanizm POTWIERDZONY ilościowo (badanie S3
## mlock-variant); ftrace zbędny — rozstrzygnęły dane sondy

### Ustalenie wstępne bez tracingu

Ponowna analiza danych S2 (sloty wokół początku drugiej serii)
rozstrzygnęła mechanizm szybciej niż planowany ftrace: w każdym
powtórzeniu slot POPRZEDZAJĄCY serię ma **czyste wake (0,03 ms) i
czysty compute (~2 ms), ale e2e = 41,9–42,2 ms**. Stall siedzi więc
wewnątrz `boradcast()` wątku przetwarzającego — między końcem
processRows() a końcem emisji: to pierwsza emisja do świeżo
zarejestrowanego klienta, czyli `message_queue(open_only)`
(executorsm.cpp:503) — shm_open+mmap segmentu kolejki. Rozmiar
segmentu potwierdzony na workerze: **3,77 MB** (3600 komunikatów
× 1 KB + indeks; bufor 10 s × 360 Hz). Kolejne sloty serii to czyste
doganianie (wake dziedziczy spóźnienie, maleje o budżet−compute).

### Badanie S3 (mlock-variant): warianty RDB_MLOCKALL, 3×3 przebiegi

Na branchu dodano przełącznik `RDB_MLOCKALL` w rtActivate
(populate = status quo / onfault = MCL_ONFAULT / off = bez mlockall;
zmiana funkcjonalna za zgodą poluzowanego pkt 23). Silnik @360 Hz,
5000 próbek, klient po 2 s; wyniki
[results/40ms/study_mlock-variant/](results/40ms/study_mlock-variant/)
na branchu:

| wariant | max e2e przy attach | zdarzeń wake≥5 ms | szczyt serii |
|---|---|---|---|
| populate (3 przebiegi) | 42,0–42,5 ms | 44–47 | 39,1–40,2 ms |
| onfault (3 przebiegi) | 17,3–17,4 ms | 14–16 | 14,3–14,5 ms |
| off (3 przebiegi) | 17,3 ms | 14 | 14,4 ms |

Polityki wątków (ps -eLo w trakcie biegu): wątek przetwarzający
**FF/50**, wątek komunikacyjny **TS**, oba na rdzeniu 3 — zgodnie z
hipotezą z Fazy 0 (sched_setscheduler(0,…) obejmuje tylko wątek
wywołujący).

### Werdykt Fazy 2

1. **Populacja stron pod MCL_FUTURE potwierdzona jako składnik
   dominujący**: ~25 ms z 42 ms (60%) znika po MCL_ONFAULT;
   identyczność wariantów onfault i off (17,3 ms co do dziesiątych!)
   dowodzi, że reszta nie ma związku z mlockall.
2. **Rezydualne ~17,3 ms** to koszt samego otwarcia/konstrukcji
   kolejki w torze emisji wątku FIFO (shm_open+mmap 3,77 MB +
   pierwszy try_send; kandydat na wyjaśnienie: inicjalizacja indeksu
   segmentu przez wątek komunikacyjny pod muteksem kolejki w shm —
   boost::interprocess bez dziedziczenia priorytetów — do
   potwierdzenia ftrace, jeśli będzie potrzebne).
3. Wniosek do poprawki docelowej (Faza 3): **MCL_ONFAULT jest
   konieczny, ale niewystarczający** — 17,3 ms nadal łamie budżet
   2,78 ms. Pełna poprawka = wyprowadzenie otwarcia/tworzenia kolejki
   poza tor emisji wątku RT (pre-otwarcie w wątku komunikacyjnym przy
   rejestracji `show` + przekazanie uchwytu do id2Queue_Cache, z
   synchronizacją dostępu) + MCL_ONFAULT; osobno przesunięcie kotwicy
   pętli za rtActivate (transjent startowy).

### Incydenty metodyczne (odnotowane zgodnie z regułą)

- **Fałszywy FAIL guardu binarki (2×)**: `strings | grep -q` pod
  `set -o pipefail` — grep -q zamyka potok po trafieniu, strings
  dostaje SIGPIPE (kod 141), pipefail uznaje potok za nieudany.
  Poprawka: grep bezpośrednio na pliku binarnym (commit 4eda822).
- **Samoaktualizacja skryptu w trakcie wykonania**: run_40ms_phase2.sh
  robi `git reset --hard` będąc uruchomionym ze zresetowanego pliku —
  bash kontynuował STARĄ wersję (stary inode), więc poprawka guardu
  nie weszła przy drugim podejściu. Obejście: aktualizacja brancha na
  workerze PRZED wywołaniem skryptu (tak robi start_supervisor.sh —
  wzorzec do zachowania).

Faza 2 zamknięta. Decyzja do podjęcia: implementacja poprawki
docelowej (Faza 3) i po jej walidacji squash brancha do mastera. Artykuł: rozdział 7 kompletny;
poza zakresem procesu pozostają TODO-ANON/TODO-CCS/TODO-TITLE
(kwestie redakcyjne submisji, nie pomiarów).
