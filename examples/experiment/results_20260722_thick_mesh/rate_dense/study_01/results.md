# Wyniki badania 1 -- kampania rate_dense

- data: 2026-07-21T23:53:16+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1310.8 us
p99            : 1605.3 us
max            : 1684.2 us
budżet (1/360s): 2777.8 us
max / budżet   : 60.6 %   (MIEŚCI SIĘ)
przepustowość  : 754 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1370.7 us
p99            : 1704.8 us
p99,9          : 1736.3 us
max            : 1788.2 us
max / budżet   : 64.4 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 21.1 us
p99            : 26.7 us
p99,9          : 33.6 us
max            : 39.7 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.54 mem_used_kb=342385 temp_mC=39654
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
