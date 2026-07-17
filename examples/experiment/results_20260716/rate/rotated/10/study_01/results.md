# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T23:03:55+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1888.2 us
p99            : 2276.7 us
max            : 2679.6 us
budżet (1/360s): 2777.8 us
max / budżet   : 96.5 %   (MIEŚCI SIĘ)
przepustowość  : 525 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1953.3 us
p99            : 2683.2 us
p99,9          : 38061.5 us
max            : 49168.7 us
max / budżet   : 1770.1 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 22.4 us
p99            : 437.3 us
p99,9          : 35640.8 us
max            : 47234.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.01 mem_used_kb=284140 temp_mC=39212
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
