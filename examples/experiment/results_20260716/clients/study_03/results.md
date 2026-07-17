# Wyniki badania 3 -- kampania clients

- data: 2026-07-16T23:38:10+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 3 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1949.3 us
p99            : 2309.6 us
max            : 2781.3 us
budżet (1/360s): 2777.8 us
max / budżet   : 100.1 %   (PRZEKROCZONY)
przepustowość  : 509 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 2028.8 us
p99            : 4779.0 us
p99,9          : 105517.0 us
max            : 118111.0 us
max / budżet   : 4252.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 23.6 us
p99            : 2634.4 us
p99,9          : 102612.9 us
max            : 115825.7 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.96 mem_used_kb=287538 temp_mC=39415
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
