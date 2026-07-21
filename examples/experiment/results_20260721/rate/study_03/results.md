# Wyniki badania 3 -- kampania rate

- data: 2026-07-21T20:12:33+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1281.3 us
p99            : 1525.9 us
max            : 43628.6 us
budżet (1/1080s): 925.9 us
max / budżet   : 4711.9 %   (PRZEKROCZONY)
przepustowość  : 754 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 4680470.0 us
p99            : 9322423.9 us
p99,9          : 9394579.5 us
max            : 9402818.5 us
max / budżet   : 1015504.4 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 4679183.8 us
p99            : 9321126.3 us
p99,9          : 9393278.9 us
max            : 9401517.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.20 mem_used_kb=286193 temp_mC=37701
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
