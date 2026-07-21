# Wyniki badania 3 -- kampania rate_dense

- data: 2026-07-21T23:56:48+02:00
- czestosc napływu: 600 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1315.8 us
p99            : 1536.5 us
max            : 1770.0 us
budżet (1/600s): 1666.7 us
max / budżet   : 106.2 %   (PRZEKROCZONY)
przepustowość  : 753 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1381.1 us
p99            : 2173.4 us
p99,9          : 2486.9 us
max            : 6056.8 us
max / budżet   : 363.4 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 24.0 us
p99            : 655.2 us
p99,9          : 951.3 us
max            : 4585.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.62 mem_used_kb=291824 temp_mC=39631
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
