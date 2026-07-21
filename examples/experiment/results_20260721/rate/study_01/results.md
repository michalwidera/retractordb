# Wyniki badania 1 -- kampania rate

- data: 2026-07-21T20:00:16+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1333.7 us
p99            : 1614.2 us
max            : 2077.9 us
budżet (1/360s): 2777.8 us
max / budżet   : 74.8 %   (MIEŚCI SIĘ)
przepustowość  : 742 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1398.3 us
p99            : 1713.5 us
p99,9          : 1880.5 us
max            : 2175.0 us
max / budżet   : 78.3 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 20.9 us
p99            : 26.1 us
p99,9          : 33.4 us
max            : 244.1 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.72 mem_used_kb=336335 temp_mC=40361
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
