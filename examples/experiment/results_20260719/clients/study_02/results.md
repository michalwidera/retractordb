# Wyniki badania 2 -- kampania clients

- data: 2026-07-18T17:33:54+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 2 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1962.3 us
p99            : 2318.4 us
max            : 2770.9 us
budżet (1/360s): 2777.8 us
max / budżet   : 99.8 %   (MIEŚCI SIĘ)
przepustowość  : 506 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 2043.6 us
p99            : 2699.1 us
p99,9          : 2844.6 us
max            : 3342.5 us
max / budżet   : 120.3 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 24.0 us
p99            : 443.3 us
p99,9          : 589.7 us
max            : 959.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.86 mem_used_kb=285348 temp_mC=39791
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
