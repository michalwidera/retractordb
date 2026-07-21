# Wyniki badania 3 -- kampania clients

- data: 2026-07-21T23:21:08+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 3 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1386.1 us
p99            : 1666.5 us
max            : 1884.1 us
budżet (1/360s): 2777.8 us
max / budżet   : 67.8 %   (MIEŚCI SIĘ)
przepustowość  : 717 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1457.1 us
p99            : 1776.3 us
p99,9          : 1811.5 us
max            : 1935.2 us
max / budżet   : 69.7 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 21.7 us
p99            : 26.4 us
p99,9          : 31.1 us
max            : 44.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.18 mem_used_kb=306243 temp_mC=39111
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
