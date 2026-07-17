# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T19:46:36+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 601
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1873.0 us
p99            : 2461.0 us
max            : 5942.4 us
budżet (1/1080s): 925.9 us
max / budżet   : 641.8 %   (PRZEKROCZONY)
przepustowość  : 536 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 661255.5 us
p99            : 1324715.0 us
p99,9          : 1340348.8 us
max            : 1340348.8 us
max / budżet   : 144757.7 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 657624.2 us
p99            : 1321271.9 us
p99,9          : 1337079.2 us
max            : 1337079.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.78 mem_used_kb=264576 temp_mC=37728
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
