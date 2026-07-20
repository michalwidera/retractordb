# Inwestygacja: speed_improvement

**Cel:** Przyspieszyć przetwarzanie danych w `xretractor` bez zmiany
funkcjonalności (identyczne wyniki na tym samym wejściu).

**Gotowe gdy:** Wprowadzona zmiana kodu obniża koszt rdzenia obliczeń
`processRows()` względem baseline, przy niepogorszonym ogonie (p99), a wynik
strumienia pozostaje bit-w-bit identyczny.

**Branch:** `investigation/speed_improvement`
**Katalog planów:** `examples/investigation/speed_improvement/`

---

## Metryka bazowa (baseline)

**Co mierzymy: E1 — koszt jednego wywołania `proc.processRows()` na interwał
(`compute_ns`).** To czysty rdzeń obliczeń silnika (construct input payload →
construct output payload → write → rules), **bez** pacing/sleep pętli i **bez**
emisji IPC (`boradcast`). Optymalizacja rdzenia widoczna jest w tej liczbie 1:1.

Metryka opiera się na **istniejącej** sondzie `RDB_BENCH_PROBE`
(`src/retractor/lib/executorsm.cpp`) i analizatorze `examples/ecg/e1_stats.py`.
Nie dodajemy nowej infrastruktury pomiarowej do silnika — sonda już mierzy
dokładnie to, czego potrzebujemy.

### Wskaźniki

| Wskaźnik | Definicja | Rola |
|---|---|---|
| **p50 compute** | mediana `compute_ns` | główny — typowy koszt interwału |
| **p99 compute** | 99. percentyl `compute_ns` | strażnik ogona (anty-regres) |
| przepustowość | N / Σ`compute_ns` (unpaced) | pochodna, próbek/s |

### Workload (stały, deterministyczny)

- RQL: `examples/ecg/rec205/rec205-detect.rql` (Pan-Tompkins, MIT-BIH 205 @360 Hz)
- Wejście: `examples/ecg/rec205/205.dat` (650 000 próbek — cały rekord)
- Uruchomienie: `-k -m 650000`, **bez `-t`** (unpaced).

### Dlaczego ten wybór

- `compute_ns` mierzy WYŁĄCZNIE `processRows()` — nie zawiera sleepu ani IPC,
  więc pacing/planista nie zaszumia pomiaru rdzenia.
- Bez `-t`: nie wymaga roota/SCHED_FIFO (działa na WSL2); `compute_ns` i tak nie
  zależy od pacingu — pacing wpływa tylko na `wake_lag`/`e2e`.
- Wejście z pliku jest deterministyczne → run powtarzalny.
- Mediana jest odporna na szum OS; p99 pilnuje, by optymalizacja nie poprawiła
  mediany kosztem ogona.

### Protokół powtarzalności

- 5 przebiegów tego samego runu; raportujemy medianę-z-median (p50) oraz rozrzut
  między przebiegami (min–max p50) jako pasmo szumu.
- Ten sam kernel/binarka; jeśli dostępne — `taskset -c <N>` na jeden rdzeń.
- Baseline i wariant z poprawką budowane tym samym `buildrdb.sh probe`.

### Reguła decyzji (poprawka poprawia / psuje efektywność)

- **AKCEPTUJ**, gdy p50 compute spada poza pasmo szumu między-runowego **i** p99
  nie rośnie istotnie **i** wynik strumienia jest bit-w-bit identyczny z baseline.
- **ODRZUĆ**, gdy p50 rośnie, albo p99 rośnie istotnie, albo zmienia się wynik.

Weryfikacja niezmienności wyniku: hash strumienia wynikowego
(`STREAM_*` / plik wynikowy) baseline vs wariant — muszą być identyczne.

---

## Kroki inwestygacji

1. **Baseline** — zbuduj `probe`, zbierz 5× E1 na workloadzie, zapisz do
   `results/baseline.md`. *Kryterium sukcesu:* stabilne p50 (rozrzut < ~10%).
2. **Przegląd kodu** — znajdź w gorącej ścieżce (`processRows` →
   `constructInputPayload` / `constructOutputPayload` / `write` /
   `constructRulesAndUpdate`) miejsca zmiany bez zmiany funkcjonalności.
   *Kryterium:* lista kandydatów z uzasadnieniem kosztu.
3. **Poprawka** — wprowadź jedną zmianę naraz (izolacja przyczyny).
4. **Pomiar + werdykt** — re-run E1, porównaj z baseline wg reguły decyzji,
   zapisz do `results/<kandydat>.md`.
