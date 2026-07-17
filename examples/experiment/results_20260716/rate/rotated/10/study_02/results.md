# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T23:10:18+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1895.6 us
p99            : 2276.1 us
max            : 43940.3 us
budżet (1/720s): 1388.9 us
max / budżet   : 3163.7 %   (PRZEKROCZONY)
przepustowość  : 508 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 6865685.4 us
p99            : 13721508.7 us
p99,9          : 13827264.9 us
max            : 13838915.7 us
max / budżet   : 996401.9 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 6863759.8 us
p99            : 13719567.7 us
p99,9          : 13825327.8 us
max            : 13836995.6 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.66 mem_used_kb=292359 temp_mC=39433
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
