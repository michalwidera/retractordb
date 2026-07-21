# speed_improvement — wskazówki do dalszych poszukiwań (notatka dla siebie)

Stan: zaakceptowano C1+C2 (kopie query 35→0/interwał, E1 p50 −11.3%), odrzucono
C3 (small_vector — mikro-alokacje nie są wąskim gardłem). Baseline po C1+C2:
**p50 ≈ 282 µs** (to jest nowy punkt odniesienia dla kolejnych kandydatów).

## K1 — ZROBIONE (2026-07-21): small_vector w eval() zamiast deque

`std::stack<descFldVT>` (kontener domyślny = `std::deque`, alokuje mapę+blok już
przy pustej konstrukcji, ~2 alok./eval) → `std::stack<…, small_vector<…,16>>`
(inline, 0 alok. dla płytkich wyrażeń). API std::stack bez zmian, ciało eval()
nietknięte. `expressionEvaluator.cpp`.

**Dowód deterministyczny (mocny):** alokacje/interwał 1137→567 (**−50.1%**),
kubełek constructOutputPayload 590→20 (**−96.6%**), bajty 319.5→168.1 KiB.
ctest 153/153 (output bit-identyczny). E1 p50 282.1→278.3 µs (**−1.35%**, rozkłady
rozdzielone), p99 płaskie (w szumie).

**KLUCZOWY WNIOSEK: −50% alokacji dało tylko −1.35% czasu.** Reconfirm lekcji
C1/C2/C3 — małe bloki glibc są tanie; **liczba alokacji NIE jest wąskim gardłem
E1** na tym workloadzie. K1 to darmowy, czysty zysk (mniej ruchu pamięci, zero
downside) — AKCEPTUJ do merge. Ale: **dalsze polowanie na alokacje (K4/K2/K3) nie
ma sensu** — zysk czasowy byłby proporcjonalnie znikomy. Następny kierunek: profiluj
co realnie dominuje ~278 µs (arytmetyka variant/visit, getItem std::any→variant,
payload read/write), nie alokacje. Licznik alokacji zrobił swoje: wykluczył całą
klasę hipotez.

## PROFIL DETERMINISTYCZNY (2026-07-21, po K1) — gdzie NAPRAWDĘ jest czas

Callgrind (`--toggle-collect='*processRows*'`, -m 200) — instrukcje deterministyczne,
odporne na szum WSL2. Pełne tabele: `results/profile-k1-callgrind.md`. Wnioski:

**Inclusive (% instrukcji processRows):** constructInputPayload 50.4% [constructAgse
31.5%, reduceFields 11%], constructOutputPayload 40.6% [eval 21.6%], **setItem 22.2%
(największy liść, cross-cut)**, **cast<std::any> 15.9%**, storage read (revRead/read/
memoryFile) ~19/19/8.6%, visit_descFld<int,std::any> 8.5%, **resolveFieldIndexOrAbort
8.5%**. Self-top: **std::any::_Manager_internal::_S_manage 6.9%**, memcmp 6.5%,
resolveFieldIndexOrAbort 5.6%, setItem 4.5%, strcmp 3.1%.

**GŁÓWNY WNIOSEK: wąskim gardłem E1 jest type-erasure `std::any` na interfejsie
payload (~30% łącznie: cast<any> 15.9% + visit_descFld→any 8.5% + _S_manage 6.9% +
any_caster/any::operator= …).** Payload mówi w `std::any`, evaluator w `descFldVT`
(wariant) — każde getItem/setItem konwertuje tam i z powrotem. To czysty narzut CPU
(NIE alokacja — potwierdzone przez K1). Do tego string-keyed mapy (memcmp+strcmp+
map<string>[] ~13%) i przeliczanie indeksu pola per dostęp.

### Kandydaci wg profilu (nowa numeracja P*, zastępuje K*-alokacyjne)
- **P1 — `std::any` → `descFldVT` w interfejsie `payload::getItem/setItem`  ★★ W TOKU**
  (~30% processRows). Postęp E0/E1/E2 niżej. E3/E4 pozostają.
- **P2 — cache indeksu pola (`resolveFieldIndexOrAbort`)  ★ tani szybki zysk** (~8.5%).
  [payload.cc:28] woła `flatIndexToDescriptorPosition` + `flatElementCount` przy KAŻDYM
  getItem/setItem, mimo że deskryptor jest statyczny w obrębie strumienia. Memoizacja
  flatIndex→position (np. wektor lookup w Descriptor budowany raz) zetnie 5.6% self.
- **P3 — string-keyed lookupy** (~13%): `qSet[id]`, `getPayload(name)`, wewnętrzne
  `map<string,vector<vector<uint8>>>` w storage, `token::getStr_` zwraca string przez
  wartość (kopia). Cache wskaźników strumieni / referencje zamiast kopii stringów.

## P1 — POSTĘP E0/E1/E2 ZROBIONE (2026-07-21)

Migracja `std::any` → `descFldVT` w interfejsie payload, etapami (każdy: ctest
153/153 Debug **i Release** + callgrind + E1 sparowany).
- **E0 (e9a75b2):** addytywne `getItemVT`/`setItemVT` w payload + test round-trip
  (parytet z getItem/setItem dla wszystkich typów + null). Nic nie przełącza.
- **E1 (49c44ba):** `eval` PUSH_ID/PUSH_ID2 → `getItemVT` (usuwa `any_to_variant_cast`
  + pośredni any na odczycie). Instr −5.79% (300.8M→283.4M), p50 273.5→266.9 (−2.4%).
- **E2 (c031ae7):** `constructOutputPayload` → `cast<descFldVT>`+`setItemVT` (usuwa
  `std::visit→any`, `cast<std::any>`). Instr −10.5% (283.4M→253.6M — największy etap),
  p50 265.9→256.1 (−3.7%). `_Manager_internal<int>` 18.4M→7.48M.

**Skumulowanie P1 (E0+E1+E2):** instrukcje processRows 300.8M→253.6M (**−15.7%**),
E1 p50 od baseline C1+C2 282.1→256.1 (**−9.2%**). ctest 153/153 Debug+Release.

**POZOSTAJE:**
- **E3:** `reduceFieldsToPayload` (fnOp<T> na std::any, valueRet any, castAny,
  setItem) i `constructAgsePayload` (getItem/setItem w pętli okna) → descFldVT +
  setItemVT/getItemVT. constructAgse to 31.5% inclusive — dużo per-element any.
- **E4:** sprzątanie — gdy gorące ścieżki na VT, przejrzeć pozostałych callerów
  getItem/setItem(any) (presenter, dumpManager, xtrdb/xqry, facctxtsrc, cmdFieldAccess);
  zimne mogą zostać na any. Nie usuwać any-interfejsu dopóki żywy caller istnieje.

## Reguła pętli: testy w OBU trybach (Debug + Release)
CI (`.circleci/config.yml:251`) uruchamia ctest w **Release**. Debug+valgrind NIE
łapie błędów Release-only. Po zmianie: `cd build/Debug && ninja && ctest` ORAZ
`cd build/Release && ninja && ninja install && cd test && ctest -j $(nproc)`.
Przykład złapany: ciche logowanie Release (SPDLOG_ACTIVE_LEVEL=ERROR, efektywność)
wycina INFO → testy trybu usługowego (ut_uxSysTermTools, it_service_idle-*)
przeasertowywały INFO; naprawione tak, by test PODĄŻAŁ za cichym Release (asercje
INFO warunkowane build-typem), commit 30998dc. Patrz [[feedback-release-tests-in-loop]].

## P2 — ZROBIONE (2026-07-21): inline hot-path akcesorów cache deskryptora

`flatIndexToDescriptorPosition` / `flatElementCount` / `byteOffsetAtFlatIndex` były
w descriptor.cc (osobne TU) — każdy getItem/setItem wołał je cross-TU, a każde
wołało `rebuildFieldMappings()` poza TU tylko po to, by sprawdzić dirty-flag.
Przeniesione do descriptor.hpp jako `inline` z dirty-checkiem inline (`if(dirty)
rebuild()`); ciężka przebudowa i zimny błąd (`flatIndexOutOfRange`, [[noreturn]])
zostają poza TU (bez wciągania fmt/FatalError do szeroko-includowanego nagłówka).
Czysty refactor, zero zmian logiki.

**Dowód:** instrukcje processRows 312.3M→300.8M (**−3.67%**, deterministyczne;
flatIndexToDescriptorPosition 6.69M i flatElementCount 1.77M wpięte w callerów,
byteOffsetAtFlatIndex 4.23M→1.37M). ctest 153/153 bit-identyczny. E1 p50 (sparowany
probe-clean) 278.0→273.8 µs (**−1.5%, rozkłady w pełni rozdzielone** 272.9-274.8 vs
276.8-278.8), p99 w szumie. Łącznie od baseline C1+C2: 282.1→273.8 (−2.9%).

## P1 — PLAN migracji `std::any` → `descFldVT` w interfejsie payload (★★ największy lewar ~30%)

**Teza:** payload mówi w `std::any`, evaluator w `descFldVT` (wariant). Każde
getItem/setItem konwertuje any↔wariant (cast<std::any> 15.9% + visit_descFld<_,any>
8.5% + _Manager_internal::_S_manage 6.9% + any_caster + any::operator=). To czysty
narzut CPU. `cast<std::variant<…>>` (wariant→wariant) już ISTNIEJE (0.95% w profilu).

**Cel:** payload udostępnia równoległy interfejs wariantowy czytający/piszący
bajty↔`descFldVT` bez `std::any`, i przełączenie gorących callerów.

**Etapy (każdy: ctest 153/153 + pomiar callgrind/E1 sparowany PRZED następnym):**
- **E0 (addytywny, zero ryzyka):** dodać `std::optional<rdb::descFldVT> payload::getItemVT(int)`
  i `void setItemVT(int, std::optional<rdb::descFldVT>)` OBOK istniejących any-owych.
  Implementacja: to samo memcpy/getVal<T> + switch po rtype, ale read→wariant,
  write←wariant. Test round-trip: getItemVT ≡ any_to_variant_cast(getItem) dla
  każdego typu (nowy ut lub rozszerzenie ut_payload). Nic nie przełączamy.
- **E1 (ścieżka odczytu eval):** w `expressionEvaluator::eval` PUSH_ID/PUSH_ID2
  zamienić `any_to_variant_cast(getItem(...))` → `getItemVT(...)`. Usuwa any_caster
  + any_to_variant_cast po stronie odczytu. Zmierz (any_caster powinien zniknąć).
- **E2 (ścieżka zapisu SELECT — największa):** w `streamInstance::constructOutputPayload`
  zamienić `std::visit→std::any→cast<std::any>→setItem(any)` → `cast<descFldVT>` +
  `setItemVT(i, …)`. Usuwa cast<std::any> 15.9% + visit_descFld<_,std::any> 8.5%.
  Uwaga: sprawdzić parytet `cast<descFldVT>` z `cast<std::any>` (null-fallback,
  szerokość stringa, rational) — [expressionEvaluator.cpp castFldVT już używany].
- **E3 (redukcje/okna):** `reduceFieldsToPayload` i `constructAgsePayload` używają
  std::any wewnętrznie (valueRet, castAny, setItem, fnOp<T> na any). Zmigrować do
  descFldVT (fnOp na wariancie). To dotyka constructAgse 31.5% inclusive.
- **E4 (sprzątanie):** gdy gorące ścieżki są na VT — sprawdzić pozostałych callerów
  getItem/setItem(any) (grep: presenter, dumpManager, xtrdb/xqry, ut_payload). Zimne
  ścieżki (wyświetlanie/dump) mogą zostać na any LUB też zmigrować. Nie usuwać
  any-interfejsu dopóki żywy caller istnieje.

**Ryzyka:** (1) getItem/setItem używane szeroko — grep callerów PRZED E1/E2.
(2) parytet cast<descFldVT> vs cast<std::any> — kluczowy, przetestować per typ.
(3) konwencja NULL: getItem→optional (nullopt=null) vs descFldVT monostate; ujednolicić.
**Szacowany zysk:** ~25-30% instrukcji processRows → przy obserwowanym stosunku
instr→czas (~1/3) rzędu kilku-kilkunastu % E1 — największy pojedynczy lewar.
**Model:** P1 dotyka kontraktu typów cross-file, wieloetapowo → implementować na
Opus 4.8 (silniejszy model), etap po etapie, nie hurtem.

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
