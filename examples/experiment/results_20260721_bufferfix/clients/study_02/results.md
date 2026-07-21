# Wyniki badania 2 -- kampania clients

- data: 2026-07-21T23:18:28+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 2 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1344.0 us
p99            : 1622.0 us
max            : 1745.1 us
budżet (1/360s): 2777.8 us
max / budżet   : 62.8 %   (MIEŚCI SIĘ)
przepustowość  : 738 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1409.8 us
p99            : 1726.4 us
p99,9          : 1762.0 us
max            : 1814.7 us
max / budżet   : 65.3 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 21.6 us
p99            : 26.6 us
p99,9          : 29.6 us
max            : 36.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.65 mem_used_kb=290280 temp_mC=38560
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
