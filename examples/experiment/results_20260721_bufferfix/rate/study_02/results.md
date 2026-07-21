# Wyniki badania 2 -- kampania rate

- data: 2026-07-21T23:12:00+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1282.4 us
p99            : 1504.8 us
max            : 1680.0 us
budżet (1/720s): 1388.9 us
max / budżet   : 121.0 %   (PRZEKROCZONY)
przepustowość  : 774 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 93515.2 us
p99            : 250554.2 us
p99,9          : 262209.7 us
max            : 263636.8 us
max / budżet   : 18981.8 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 92225.6 us
p99            : 249265.1 us
p99,9          : 260919.4 us
max            : 262345.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.67 mem_used_kb=279208 temp_mC=37683
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
