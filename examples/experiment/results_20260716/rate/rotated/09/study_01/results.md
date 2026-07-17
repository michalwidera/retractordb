# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T22:33:41+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1865.4 us
p99            : 2242.8 us
max            : 2635.3 us
budżet (1/360s): 2777.8 us
max / budżet   : 94.9 %   (MIEŚCI SIĘ)
przepustowość  : 533 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1932.4 us
p99            : 2518.5 us
p99,9          : 29913.3 us
max            : 43288.7 us
max / budżet   : 1558.4 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 22.0 us
p99            : 357.4 us
p99,9          : 26951.1 us
max            : 40930.6 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.75 mem_used_kb=302892 temp_mC=41537
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
