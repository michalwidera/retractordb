# Wyniki badania 3 -- kampania rate_fine

- data: 2026-07-22T00:10:31+02:00
- czestosc napływu: 450 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1335.4 us
p99            : 1598.2 us
max            : 1845.6 us
budżet (1/450s): 2222.2 us
max / budżet   : 83.1 %   (MIEŚCI SIĘ)
przepustowość  : 749 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1394.9 us
p99            : 1696.5 us
p99,9          : 1766.5 us
max            : 1992.5 us
max / budżet   : 89.7 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 21.8 us
p99            : 29.4 us
p99,9          : 34.2 us
max            : 45.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.24 mem_used_kb=291161 temp_mC=39410
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
