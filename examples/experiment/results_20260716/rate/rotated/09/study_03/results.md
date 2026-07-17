# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T22:46:23+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1791.6 us
p99            : 2151.0 us
max            : 42276.5 us
budżet (1/1080s): 925.9 us
max / budżet   : 4565.9 %   (PRZEKROCZONY)
przepustowość  : 537 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 10039046.8 us
p99            : 19859903.6 us
p99,9          : 20023386.5 us
max            : 20041232.5 us
max / budżet   : 2164453.1 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 10037240.3 us
p99            : 19858104.7 us
p99,9          : 20021589.5 us
max            : 20039434.9 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.68 mem_used_kb=278758 temp_mC=39723
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
