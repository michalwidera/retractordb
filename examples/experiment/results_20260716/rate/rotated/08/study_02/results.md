# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T22:06:16+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1735.8 us
p99            : 2094.8 us
max            : 42148.4 us
budżet (1/720s): 1388.9 us
max / budżet   : 3034.7 %   (PRZEKROCZONY)
przepustowość  : 555 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 4773963.0 us
p99            : 9494120.7 us
p99,9          : 9563597.5 us
max            : 9570988.6 us
max / budżet   : 689111.2 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 4772223.5 us
p99            : 9453441.1 us
p99,9          : 9561869.3 us
max            : 9569254.0 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.00 mem_used_kb=285017 temp_mC=39798
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
