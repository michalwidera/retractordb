# Wyniki badania 1 -- kampania clients

- data: 2026-07-21T23:15:43+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1350.5 us
p99            : 1641.8 us
max            : 1748.8 us
budżet (1/360s): 2777.8 us
max / budżet   : 63.0 %   (MIEŚCI SIĘ)
przepustowość  : 733 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1412.5 us
p99            : 1740.2 us
p99,9          : 1778.6 us
max            : 1859.6 us
max / budżet   : 66.9 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 22.2 us
p99            : 27.4 us
p99,9          : 32.9 us
max            : 38.9 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.92 mem_used_kb=284847 temp_mC=38193
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
