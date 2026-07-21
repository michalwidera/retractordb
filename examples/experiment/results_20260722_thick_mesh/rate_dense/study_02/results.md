# Wyniki badania 2 -- kampania rate_dense

- data: 2026-07-21T23:55:04+02:00
- czestosc napływu: 540 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1312.1 us
p99            : 1573.6 us
max            : 1772.2 us
budżet (1/540s): 1851.9 us
max / budżet   : 95.7 %   (MIEŚCI SIĘ)
przepustowość  : 755 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1374.8 us
p99            : 1990.9 us
p99,9          : 2359.4 us
max            : 2630.0 us
max / budżet   : 142.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 22.3 us
p99            : 486.3 us
p99,9          : 762.2 us
max            : 921.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.15 mem_used_kb=290511 temp_mC=39704
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
