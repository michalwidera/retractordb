# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T19:23:35+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1984.5 us
p99            : 2521.4 us
max            : 2850.6 us
budżet (1/360s): 2777.8 us
max / budżet   : 102.6 %   (PRZEKROCZONY)
przepustowość  : 495 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 28663430.2 us
p99            : 56801667.3 us
p99,9          : 57319250.1 us
max            : 57374045.1 us
max / budżet   : 2065465.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 28659907.9 us
p99            : 56797605.0 us
p99,9          : 57315827.9 us
max            : 57370573.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.23 mem_used_kb=305763 temp_mC=42675
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
