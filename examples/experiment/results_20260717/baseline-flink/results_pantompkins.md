# Wyniki kampanii baseline-flink -- krok 2: rownowazny dataflow Pan-Tompkinsa

- data: 2026-07-17T21:18:52+02:00
- cel: 7.5(ii) main-debs.tex -- porownanie modelu wykonania DSMS glownego
  nurtu (Flink DataStream API) z silnikiem RetractorDB i baseline-numpy
- Flink: 2.3.0, tryb local/MiniCluster (jeden JVM embedded w
  `bin/flink run -t local`), parallelism=1, checkpointing wylaczony
- limity pamieci: jobmanager.memory.process.size=768m,
  taskmanager.memory.process.size=1024m
- srodowisko: 6.8.0-2049-raspi-realtime, governor performance, taskset -c 3
- **ODSTEPSTWO**: SCHED_FIFO NIE nadany JVM-owi (wielowatkowy GC/JIT --
  ryzyko jak w Kampanii 7, patrz komentarz w skrypcie); tylko
  governor+taskset, tak samo jak reszta srodowiska
- exit_code przebiegu: 0

## Wersje
```
openjdk version "17.0.19" 2026-04-21
OpenJDK Runtime Environment (build 17.0.19+10-1-24.04.2-Ubuntu)
OpenJDK 64-Bit Server VM (build 17.0.19+10-1-24.04.2-Ubuntu, mixed mode, sharing)
Flink 2.3.0
```

## Log przebiegu (naglowek META + koniec)
```
META rec_len=650000 bp_len=25 d_len=5 rate_hz=360.0 samples=20000
META rec_len=650000 bp_len=25 d_len=5 rate_hz=360.0 samples=20000
Job has been submitted with JobID d20a3a128e1ab4b8bc47ff981c7bc81c
Program execution finished
Job with JobID d20a3a128e1ab4b8bc47ff981c7bc81c has finished.
Job Runtime: 60421 ms

PROBE /dev/shm/flink-pantompkins/probe_flink.csv
```

## Analiza sondy (e1_stats.py, format RDB_BENCH_CSV)
```
plik           : /dev/shm/flink-pantompkins/probe_flink.csv
interwałów (N) : 20000
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 4.7 us
p99            : 49.1 us
max            : 54714.8 us
budżet (1/360s): 2777.8 us
max / budżet   : 1969.7 %   (PRZEKROCZONY)
przepustowość  : 81,805 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 607.7 us
p99            : 1231.3 us
p99,9          : 25470.6 us
max            : 55745.3 us
max / budżet   : 2006.8 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 593.8 us
p99            : 1105.2 us
p99,9          : 24457.2 us
max            : 52981.6 us
```

## UWAGA INTERPRETACYJNA (obowiazkowa przed cytowaniem w artykule)
compute_ns mierzy od odebrania rekordu przez zrodlo (PO pacingu) do
konca ostatniego etapu obliczen (Threshold, PRZED sinkiem). Dzieki
chainowaniu operatorow (map->map->map->map->sink, ta sama parallelism,
brak keyBy/shuffle) caly potok dziala w JEDNYM wątku -- ale definicja
compute_ns wciaz zawiera JEDNO przejscie przez kolejke Source->Chain
(Flink nie chainuje zrodel), czego nie ma numpy/silnik (tam jedna
synchroniczna funkcja). e2e_ns/wake_lag_ns moga zawierac tail od
rozgrzewki JIT (kompilacja C1/C2 w pierwszych iteracjach) -- odrebny
mechanizm od pikow housekeepingu jadra opisanych dla silnika (K6/K9).

## Metryki systemowe w trakcie badania
```
srednie load1=1.77 mem_used_kb=616860 temp_mC=39812
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- stan maszyny przed/po (krok 2)
- probe_flink.csv -- surowa sonda (iter,compute_ns,wake_lag_ns,e2e_ns)
- run_stdout.log -- pelne wyjscie `bin/flink run` (w tym META)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
- PanTompkinsFlinkJob.java -- kopia zrodla joba na moment przebiegu
