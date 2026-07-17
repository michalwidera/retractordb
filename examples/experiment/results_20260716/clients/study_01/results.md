# Wyniki badania 1 -- kampania clients

- data: 2026-07-16T23:14:48+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1890.9 us
p99            : 2264.7 us
max            : 2761.1 us
budżet (1/360s): 2777.8 us
max / budżet   : 99.4 %   (MIEŚCI SIĘ)
przepustowość  : 524 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1956.2 us
p99            : 2658.8 us
p99,9          : 37490.9 us
max            : 48345.0 us
max / budżet   : 1740.4 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 23.2 us
p99            : 425.4 us
p99,9          : 35531.5 us
max            : 46440.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.25 mem_used_kb=284565 temp_mC=39635
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
