# Wyniki badania 1 -- kampania rate_fine

- data: 2026-07-22T00:06:34+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1307.6 us
p99            : 1614.0 us
max            : 1698.0 us
budżet (1/360s): 2777.8 us
max / budżet   : 61.1 %   (MIEŚCI SIĘ)
przepustowość  : 756 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1367.5 us
p99            : 1712.2 us
p99,9          : 1749.3 us
max            : 1780.4 us
max / budżet   : 64.1 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 20.5 us
p99            : 25.7 us
p99,9          : 34.2 us
max            : 38.4 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.57 mem_used_kb=285505 temp_mC=38220
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
