# Werdykt inwestygacji: speed_improvement — kandydat C1+C2

**Poprawka:** eliminacja zbędnych kopii obiektu `query` w gorącej ścieżce
`processRows` (kopia → referencja, ten sam odczyt, zero zmian funkcjonalności).

- **C1** `dataModel.cpp` `constructInputPayload:152`:
  `auto qry = coreInstance_[instance]` → `const query &qry = coreInstance_[instance]`.
- **C2** `dataModel.cpp` `processRows:137` (pętla deklaracji):
  `for (auto q : coreInstance_)` → `for (const auto &q : coreInstance_)`.

Mechanizm: `qTree::operator[]` zwraca `query&`; `auto`/`for(auto q)` kopiowały
cały obiekt (3 listy: lProgram/lSchema/lRules + stringi + pary) na każdy
strumień/interwał.

## Dowody

### 1. Deterministyczny — licznik kopii (niezależny od WSL2/zegara)

| wariant | kopie `query` / interwał |
|---|---|
| baseline | **35.000** |
| po C1+C2 | **0.000** |

(Szczegóły: `results/copy-count.md`.)

### 2. Poprawność — pełny pakiet testów

`ctest` (Debug, unit+integration): **153/153 passed, 0 failed** — w tym
`ut_dataModel-compile` i `ut_dataModel-compare` (bezpośrednio ćwiczą zmieniony
plik). Brak zmiany funkcjonalności potwierdzony regresją.

### 3. Czas (E1 = compute_ns processRows) — pomiar pomocniczy

Sparowany, wyciszona maszyna, 5×20000 interwałów, unpaced, taskset -c 3.
**Uwaga: WSL2 — pomiary czasowe traktowane jako potwierdzenie, nie dowód
główny.**

| wskaźnik | baseline (paired) | opt-query-copy | Δ |
|---|---|---|---|
| **p50 compute** | 317.6 µs (316.4–320.0) | **281.7 µs (280.7–284.9)** | **−35.9 µs (−11.3%)** |
| p99 compute | 834.8 µs | 762.1 µs | −72.7 µs (−8.7%) |

Pasma p50 rozdzielone luką ~31 µs (brak nakładania). p99 również spada → brak
regresji ogona.

## Reguła decyzji → **AKCEPTUJ**

- p50 spada daleko poza pasmo szumu ✔
- p99 nie rośnie (spada) ✔
- wynik bit-w-bit niezmieniony (153/153 testów + 0 kopii to inny odczyt tych
  samych danych) ✔

## Rekomendacja przeniesienia do master

Do master trafia **wyłącznie zmiana C1+C2 w `dataModel.cpp`** (2 linie,
`git apply` z `scratchpad/fix.patch` lub cherry-pick pojedynczego commita).
Rusztowanie inwestygacji (licznik `RDB_COPY_COUNTER`, harness, wyniki) zostaje
na branchu.

## Sprawdzone i odrzucone

- **C3** — `small_vector` zamiast `std::vector<token> arg`: brak mierzalnego
  zysku (p50 282.1 vs 281.7 µs, w szumie). Cofnięte. Szczegóły:
  `results/C3-smallvec-REJECTED.md`. Wniosek: narzut kopii był w kopiach całego
  `query` (C1+C2), nie w mikro-alokacjach.

## Dalszy kierunek (nie wdrożono)

- Kopie payloadów `*(inputPayload) = *getPayload(...)` — konieczne (kopiują
  dane), ale warto sprawdzić czy nie ma zbędnych pośrednich alokacji.
- Ścieżka `constructOutputPayload` / `write` — większe struktury niż mikro-alokacje.
