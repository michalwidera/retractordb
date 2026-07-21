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

## LICZNIK ALOKACJI — ZROBIONE (2026-07-21)

Dodany `RDB_ALLOC_COUNTER` (opcja CMake, `src/retractor/allocCounter.cpp` — wprost
w binarce, nie w libie, żeby zamiana globalnego `operator new` na pewno się
dolinkowała). Raport na stderr przy wyjściu: `allocs/bytes/frees total`. Harness:
`run_alloc.sh <label> [N1] [N2]` — metoda różnicy (domyślnie N1=1000, N2=6000).

**BASELINE (deterministyczny, HEAD investigation/speed_improvement):**
- **alokacje / interwał = 1137.01** (różnica dokładnie stała między przebiegami —
  szum startowy ±kilkanaście kasuje się w różnicy; potwierdzone 2×)
- **bajty / interwał = 319.5 KiB**
- **zwolnienia / interwał = 1137.01** = allocs → stan ustalony (brak akumulacji),
  a spójność frees==allocs potwierdza, że niewyrównana rodzina new/delete łapie
  obie strony (typy nadwyrównane nie występują na gorącej ścieżce).

~1137 alloc/interwał ÷ ~15 strumieni ≈ 76 alloc/strumień — to twardy cel. Teraz
JEDEN kandydat naraz: `run_alloc.sh k1-...` przed/po → różnica alokacji = dowód.

### Build wariantu z licznikiem
```bash
scripts/buildrdb.sh probe
( cd build/Release && cmake -DRDB_ALLOC_COUNTER=ON . && ninja xretractor )
```
UWAGA: atomic per alloc perturbuje czas → E1 (czas) mierz na czystym `probe`
(bez ALLOC_COUNTER), alokacje mierz na build z ALLOC_COUNTER. Dwa osobne buildy.

### LOKALIZACJA — ZROBIONE (2026-07-21)
Dodane kubełki `RDB_ALLOC_SCOPE` (`allocCounter.hpp` + rejestr w `allocCounter.cpp`;
instrumentacja faz `processRows` w `dataModel.cpp`, cała pod `#ifdef` → produkcja
neutralna, ut_dataModel/payload OK). `run_alloc.sh` wypisuje teraz tabelę kubełków.
Wyniki **idealnie deterministyczne** (każda delta dzieli się bez reszty przez Δ):

| faza processRows | alloc/interwał | udział processRows |
|---|---|---|
| **K1 `constructOutputPayload`** | **590** | **52.8%** ← cel #1 |
| `constructInputPayload` (całość) | 465 | 41.6% |
| — **K4 `constructAgsePayload`** | **264** | 23.6% ← cel #2 |
| — K2 `reduceFieldsToPayload` | 60 | 5.4% |
| — inne (getPayload/operator+/DEHASH) | 141 | 12.6% |
| `outputPayload.write` | 62 | 5.5% |
| `declarations` | 1 | 0.1% |
| `constructRulesAndUpdate` | 0 | 0% |

Suma faz = 1118/1137 (~19/interwał poza `processRows`: getAwaitedStreamsSet/boradcast).
**Wniosek: K1 (590) + K4 (264) = 854 = 75% wszystkich alokacji.** K1 to
jednoznaczny cel #1, K4 drugi. K2 (60) i K3 (inne=141) mają mały udział — niski
priorytet. Atakuj K1, mierz `run_alloc.sh k1-... ` przed/po (różnica kubełka
constructOutputPayload = dowód) + E1 czas na osobnym buildzie probe.

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

### K2 — `reduceFieldsToPayload` (SUM/AVG/MIN/MAX)  — NISKI PRIORYTET (60/interwał, 5.4%)
[streamInstance.cpp:179]. Per wywołanie: `std::make_unique<rdb::payload>` (alloc),
`std::any valueRet` + `castAny` per element, `fnOp<T>` na `std::any`. Używane
przez mwi (.avg), mwi_thr (.avg), bp_out (.sumc) → co interwał.
Hipoteza: usunąć `std::any` z pętli redukcji (trzymać `descFldVT`/konkretny typ),
zwracać payload bez `make_unique` (przez wartość, NRVO — jak w constructAgsePayload
po jego optymalizacji). Zmierz alokacje.

### K3 — kopie payloadów wejściowych  — NISKI PRIORYTET (~141/interwał w "inne", 12.6%)
[dataModel.cpp constructInputPayload]: `*(inputPayload) = *getPayload(a)` oraz
`*getPayload(a) + *getPayload(b)` (STREAM_ADD, l. ~254). Kopiują dane (konieczne),
ale sprawdź `payload::operator+` i `operator=` pod kątem ZBĘDNYCH pośrednich
alokacji/kopii (np. tymczasowy payload w operator+). Cel: eliminacja tymczasowych,
nie samych danych.

### K4 — `constructAgsePayload` (okna FIR)  ★ CEL #2 (264/interwał, 23.6%)
Już ma cache deskryptora. Sprawdź czy `getItem`/`setItem` w pętli po elementach
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
