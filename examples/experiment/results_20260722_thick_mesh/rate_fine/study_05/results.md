# Wyniki badania 5 -- kampania rate_fine

- data: 2026-07-22T00:14:18+02:00
- czestosc napływu: 510 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_5/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1323.4 us
p99            : 1575.6 us
max            : 1845.6 us
budżet (1/510s): 1960.8 us
max / budżet   : 94.1 %   (MIEŚCI SIĘ)
przepustowość  : 749 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1386.3 us
p99            : 1820.5 us
p99,9          : 2278.9 us
max            : 2424.4 us
max / budżet   : 123.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 22.5 us
p99            : 443.9 us
p99,9          : 699.6 us
max            : 821.3 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.20 mem_used_kb=286109 temp_mC=39805
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
