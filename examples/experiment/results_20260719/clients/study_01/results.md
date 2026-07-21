# Wyniki badania 1 -- kampania clients

- data: 2026-07-18T17:17:14+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1888.3 us
p99            : 2290.3 us
max            : 2786.5 us
budżet (1/360s): 2777.8 us
max / budżet   : 100.3 %   (PRZEKROCZONY)
przepustowość  : 524 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1953.6 us
p99            : 2642.2 us
p99,9          : 2789.1 us
max            : 3281.7 us
max / budżet   : 118.1 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 22.7 us
p99            : 406.9 us
p99,9          : 495.4 us
max            : 971.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.23 mem_used_kb=284300 temp_mC=39075
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
