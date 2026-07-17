# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T22:12:37+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1731.1 us
p99            : 2082.3 us
max            : 42097.6 us
budżet (1/1080s): 925.9 us
max / budżet   : 4546.5 %   (PRZEKROCZONY)
przepustowość  : 557 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 9351476.3 us
p99            : 18561806.7 us
p99,9          : 18713632.6 us
max            : 18730276.4 us
max / budżet   : 2022869.9 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 9349741.4 us
p99            : 18560064.4 us
p99,9          : 18711891.3 us
max            : 18728555.3 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.03 mem_used_kb=289542 temp_mC=39690
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
