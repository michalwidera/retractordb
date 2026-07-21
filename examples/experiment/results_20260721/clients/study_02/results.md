# Wyniki badania 2 -- kampania clients

- data: 2026-07-21T20:34:37+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 2 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1330.0 us
p99            : 1620.4 us
max            : 2110.7 us
budżet (1/360s): 2777.8 us
max / budżet   : 76.0 %   (MIEŚCI SIĘ)
przepustowość  : 744 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1396.4 us
p99            : 1724.8 us
p99,9          : 1872.7 us
max            : 2208.5 us
max / budżet   : 79.5 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 21.2 us
p99            : 26.7 us
p99,9          : 33.9 us
max            : 272.4 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.06 mem_used_kb=292295 temp_mC=37108
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
