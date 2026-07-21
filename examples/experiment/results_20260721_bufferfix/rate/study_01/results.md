# Wyniki badania 1 -- kampania rate

- data: 2026-07-21T23:00:54+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1331.0 us
p99            : 1618.3 us
max            : 1999.6 us
budżet (1/360s): 2777.8 us
max / budżet   : 72.0 %   (MIEŚCI SIĘ)
przepustowość  : 744 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1391.8 us
p99            : 1717.2 us
p99,9          : 1759.2 us
max            : 2148.1 us
max / budżet   : 77.3 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 20.8 us
p99            : 25.9 us
p99,9          : 31.8 us
max            : 47.1 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.49 mem_used_kb=313730 temp_mC=39020
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
