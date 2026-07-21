# Wyniki badania 4 -- kampania rate_dense

- data: 2026-07-21T23:58:30+02:00
- czestosc napływu: 660 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_4/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1278.6 us
p99            : 1496.9 us
max            : 1761.9 us
budżet (1/660s): 1515.2 us
max / budżet   : 116.3 %   (PRZEKROCZONY)
przepustowość  : 775 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1617.7 us
p99            : 4070.9 us
p99,9          : 6798.5 us
max            : 8014.8 us
max / budżet   : 529.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 337.3 us
p99            : 2598.5 us
p99,9          : 5383.0 us
max            : 6498.7 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.51 mem_used_kb=299819 temp_mC=39970
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
