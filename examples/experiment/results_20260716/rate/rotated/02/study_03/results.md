# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T15:30:11+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 5531.8 us
p99            : 6316.2 us
max            : 8835.5 us
budżet (1/1080s): 925.9 us
max / budżet   : 954.2 %   (PRZEKROCZONY)
przepustowość  : 180 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 77156040.6 us
p99            : 152877653.0 us
p99,9          : 154267015.6 us
max            : 154412515.7 us
max / budżet   : 16676551.7 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 77147383.3 us
p99            : 152868917.2 us
p99,9          : 154258525.1 us
max            : 154403929.3 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.63 mem_used_kb=269302 temp_mC=41496
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
