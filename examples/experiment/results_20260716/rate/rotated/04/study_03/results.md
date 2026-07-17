# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T17:29:28+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 3024.6 us
p99            : 3668.4 us
max            : 6345.5 us
budżet (1/1080s): 925.9 us
max / budżet   : 685.3 %   (PRZEKROCZONY)
przepustowość  : 329 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 43596644.4 us
p99            : 86458339.0 us
p99,9          : 87243975.6 us
max            : 87326480.4 us
max / budżet   : 9431259.9 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 43591297.2 us
p99            : 86453137.2 us
p99,9          : 87238866.9 us
max            : 87321248.4 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.29 mem_used_kb=271926 temp_mC=40952
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
