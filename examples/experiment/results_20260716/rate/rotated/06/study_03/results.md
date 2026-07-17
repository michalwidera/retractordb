# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T20:41:16+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1763.0 us
p99            : 2164.5 us
max            : 43001.2 us
budżet (1/1080s): 925.9 us
max / budżet   : 4644.1 %   (PRZEKROCZONY)
przepustowość  : 546 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 22376757.6 us
p99            : 44258496.1 us
p99,9          : 44682365.9 us
max            : 44722585.8 us
max / budżet   : 4830039.3 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 22373783.2 us
p99            : 44255526.3 us
p99,9          : 44679404.9 us
max            : 44719610.0 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.19 mem_used_kb=279289 temp_mC=38946
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
