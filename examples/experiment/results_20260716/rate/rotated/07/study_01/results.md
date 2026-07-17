# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T21:51:14+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1846.9 us
p99            : 2231.2 us
max            : 24883.8 us
budżet (1/360s): 2777.8 us
max / budżet   : 895.8 %   (PRZEKROCZONY)
przepustowość  : 545 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 17193144.7 us
p99            : 23979887.6 us
p99,9          : 24102735.8 us
max            : 24115352.5 us
max / budżet   : 868152.7 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 17190177.7 us
p99            : 23977912.9 us
p99,9          : 24100119.6 us
max            : 24113408.1 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=3.46 mem_used_kb=364357 temp_mC=42214
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
