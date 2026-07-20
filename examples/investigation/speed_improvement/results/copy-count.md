# Diagnostyka: kopie obiektu `query` na interwał (licznik RDB_COPY_COUNTER)

Metoda: build `-DRDB_COPY_COUNTER=ON`; dwa przebiegi `-m N1`/`-m N2`; kopie na
interwał = (copies(N2) − copies(N1)) / (N2 − N1). Różnica izoluje kopie pętli
`processRows` od stałych kopii startowych (parsing/kompilacja/sort topologiczny).

Workload: `rec205-detect.rql`, N1=2000, N2=22000 (Δ=20000 interwałów).

| wariant | copies@2000 | copies@22000 | **kopie/interwał** |
|---|---|---|---|
| **baseline** | 70060 | 770060 | **35.000** |
| **po C1+C2** | 95 | 95 | **0.000** |

Wniosek: poprawka C1+C2 eliminuje **wszystkie 35** kopii obiektu `query` na
interwał w gorącej ścieżce. Dowód deterministyczny — niezależny od zegara/WSL2.

Poprawki (bez zmiany funkcjonalności — kopia→referencja, ten sam odczyt):
- **C1** `dataModel.cpp` `constructInputPayload`: `auto qry = coreInstance_[instance]`
  → `const query &qry = coreInstance_[instance]`.
- **C2** `dataModel.cpp` `processRows`, pętla deklaracji:
  `for (auto q : coreInstance_)` → `for (const auto &q : coreInstance_)`.

Uwaga: `operator[]` w qTree zwraca `query&`, więc `auto` kopiował cały obiekt
(3 listy: lProgram/lSchema/lRules + stringi) — 35× na interwał.
