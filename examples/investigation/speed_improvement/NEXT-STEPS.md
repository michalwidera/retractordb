# speed_improvement — wskazówki do dalszych poszukiwań (notatka dla siebie)

Stan: zaakceptowano C1+C2 (kopie query 35→0/interwał, E1 p50 −11.3%), odrzucono
C3 (small_vector — mikro-alokacje nie są wąskim gardłem). Baseline po C1+C2:
**p50 ≈ 282 µs** (to jest nowy punkt odniesienia dla kolejnych kandydatów).

## Metodyka (potwierdzona)

- Na WSL2 czas jest zaszumiony → **głównym dowodem rób licznik deterministyczny**,
  czas to tylko potwierdzenie. Wzorzec licznika kopii (`RDB_COPY_COUNTER`,
  `query.hpp/.cpp` + `CMakeLists`) zadziałał świetnie.
- Harness: `run_e1.sh <label>` (5×20000, unpaced, taskset -c 3). Zawsze pomiar
  SPAROWANY back-to-back (baseline vs wariant), bo stan maszyny dryfuje.
- Wniosek z C1/C2/C3: **celuj w kopie/alokacje DUŻYCH struktur, nie w
  pojedyncze małe alokacje** (glibc obsługuje małe bloki tanio).

## Rekomendowany następny krok metodyczny: LICZNIK ALOKACJI

Analogicznie do `RDB_COPY_COUNTER` dodać `RDB_ALLOC_COUNTER` — globalny
`operator new`/`delete` zliczający alokacje sterty (i sumę bajtów) na interwał
(różnica m=N2 − m=N1 / Δ, jak przy kopiach). Deterministyczny, niezależny od
WSL2. Da twardą listę: ile alokacji/interwał i który wariant je usuwa. To
najszybsza droga do znalezienia realnego wąskiego gardła bez zgadywania.

## Konkretni kandydaci (od najbardziej obiecujących)

### K1 — churn `std::any` + `std::visit` w ewaluacji pól  ★ priorytet
Gorąca ścieżka `constructOutputPayload` ([streamInstance.cpp:377]):
- `std::visit([](auto&&arg)->std::any{return arg;}, retVal)` (l. ~389) opakowuje
  wynik w `std::any` (alokuje dla typów > SBO, np. string/rational),
- `cast<std::any> castAny; castAny(result, rtype)` — kolejny `std::any`.
Per pole, per interwał. W rec205-detect ~15 strumieni × kilka pól.
Hipoteza: przejście z `std::any` na `rdb::descFldVT` (wariant, bez alokacji) na
tej ścieżce zetnie alokacje. Ryzyko: dotyka `cast<>` i `setItem` — sprawdź
kontrakt typów. Zmierz licznikiem alokacji przed/po.

### K2 — `reduceFieldsToPayload` (SUM/AVG/MIN/MAX)  ★
[streamInstance.cpp:179]. Per wywołanie: `std::make_unique<rdb::payload>` (alloc),
`std::any valueRet` + `castAny` per element, `fnOp<T>` na `std::any`. Używane
przez mwi (.avg), mwi_thr (.avg), bp_out (.sumc) → co interwał.
Hipoteza: usunąć `std::any` z pętli redukcji (trzymać `descFldVT`/konkretny typ),
zwracać payload bez `make_unique` (przez wartość, NRVO — jak w constructAgsePayload
po jego optymalizacji). Zmierz alokacje.

### K3 — kopie payloadów wejściowych
[dataModel.cpp constructInputPayload]: `*(inputPayload) = *getPayload(a)` oraz
`*getPayload(a) + *getPayload(b)` (STREAM_ADD, l. ~254). Kopiują dane (konieczne),
ale sprawdź `payload::operator+` i `operator=` pod kątem ZBĘDNYCH pośrednich
alokacji/kopii (np. tymczasowy payload w operator+). Cel: eliminacja tymczasowych,
nie samych danych.

### K4 — `constructAgsePayload` / `fetchForward` zwracają payload przez wartość
Już mają cache deskryptora. Sprawdź czy `getItem`/`setItem` w pętli po elementach
nie robią alokacji per element (np. `std::any` w getItem). Splot FIR (mlii_win@(1,25),
bp_win@(1,5), mwi_win@(1,30), mwi_long@(1,180)) → duże okna, dużo elementów/interwał.
mwi_long ma okno 180 — to potencjalnie najcięższy pojedynczy strumień.

## Czego NIE ruszać (sprawdzone/bez sensu)
- small_vector dla `arg` (C3) — brak zysku.
- `expressionEvaluator` jest bezstanowy — tworzenie per pole jest darmowe.
- Kopie query — już usunięte (C1+C2).

## Jak wznowić
1. `git checkout investigation/speed_improvement`
2. `scripts/buildrdb.sh probe` (+ ewentualnie dodać `RDB_ALLOC_COUNTER`)
3. baseline: `run_e1.sh baseline` (powinno dać ~282 µs — to już stan z C1+C2 na master?
   NIE: master nie ma C1+C2 dopóki impl nie zmergowany; baseline licz na aktualnym HEAD)
4. jeden kandydat naraz → licznik alokacji + E1 sparowany → VERDICT.
