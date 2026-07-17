# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T15:21:35+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 6554.3 us
p99            : 8239.2 us
max            : 9934.8 us
budżet (1/720s): 1388.9 us
max / budżet   : 715.3 %   (PRZEKROCZONY)
przepustowość  : 149 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 101635014.2 us
p99            : 201352844.4 us
p99,9          : 203170585.0 us
max            : 203360730.5 us
max / budżet   : 14641972.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 101625846.0 us
p99            : 201341867.7 us
p99,9          : 203159321.3 us
max            : 203351258.0 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.86 mem_used_kb=266240 temp_mC=40171
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
