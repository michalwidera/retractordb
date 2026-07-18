# Wyniki badania 3 -- kampania rate

- data: 2026-07-18T16:53:29+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1845.1 us
p99            : 2227.4 us
max            : 45299.7 us
budżet (1/1080s): 925.9 us
max / budżet   : 4892.4 %   (PRZEKROCZONY)
przepustowość  : 518 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 10665054.7 us
p99            : 21157411.2 us
p99,9          : 21339654.4 us
max            : 21358623.0 us
max / budżet   : 2306731.3 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 10663205.9 us
p99            : 21155553.8 us
p99,9          : 21337808.6 us
max            : 21356782.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.65 mem_used_kb=285957 temp_mC=39720
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
