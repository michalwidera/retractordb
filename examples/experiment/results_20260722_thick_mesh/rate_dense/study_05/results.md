# Wyniki badania 5 -- kampania rate_dense

- data: 2026-07-22T00:00:10+02:00
- czestosc napływu: 700 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_5/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1277.5 us
p99            : 1493.9 us
max            : 1766.5 us
budżet (1/700s): 1428.6 us
max / budżet   : 123.7 %   (PRZEKROCZONY)
przepustowość  : 777 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1627.6 us
p99            : 5572.0 us
p99,9          : 8225.3 us
max            : 9569.8 us
max / budżet   : 669.9 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 342.6 us
p99            : 4157.3 us
p99,9          : 6745.3 us
max            : 8083.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.96 mem_used_kb=289996 temp_mC=39816
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
