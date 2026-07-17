# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T20:27:42+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1777.1 us
p99            : 2190.1 us
max            : 43266.0 us
budżet (1/360s): 2777.8 us
max / budżet   : 1557.6 %   (PRZEKROCZONY)
przepustowość  : 540 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 4036201.9 us
p99            : 8138550.1 us
p99,9          : 8221885.4 us
max            : 8227646.2 us
max / budżet   : 296195.3 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 4033161.6 us
p99            : 8135165.6 us
p99,9          : 8218869.7 us
max            : 8224641.0 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.45 mem_used_kb=288933 temp_mC=38794
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
