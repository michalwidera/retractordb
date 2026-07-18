# Wyniki badania 3 -- kampania clients

- data: 2026-07-18T17:40:33+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 3 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1952.7 us
p99            : 2325.6 us
max            : 2915.6 us
budżet (1/360s): 2777.8 us
max / budżet   : 105.0 %   (PRZEKROCZONY)
przepustowość  : 509 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 2034.4 us
p99            : 2737.2 us
p99,9          : 2876.7 us
max            : 3498.8 us
max / budżet   : 126.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 23.4 us
p99            : 460.0 us
p99,9          : 568.8 us
max            : 965.4 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.90 mem_used_kb=298969 temp_mC=40481
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
