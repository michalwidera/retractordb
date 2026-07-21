# Wyniki badania 3 -- kampania clients

- data: 2026-07-21T20:51:14+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 3 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1361.3 us
p99            : 1651.1 us
max            : 2115.2 us
budżet (1/360s): 2777.8 us
max / budżet   : 76.1 %   (MIEŚCI SIĘ)
przepustowość  : 729 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1434.0 us
p99            : 1760.7 us
p99,9          : 1912.9 us
max            : 2247.8 us
max / budżet   : 80.9 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 21.4 us
p99            : 26.6 us
p99,9          : 32.3 us
max            : 316.7 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.87 mem_used_kb=295245 temp_mC=38055
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
