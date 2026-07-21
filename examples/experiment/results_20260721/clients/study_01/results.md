# Wyniki badania 1 -- kampania clients

- data: 2026-07-21T20:18:00+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1300.7 us
p99            : 1608.7 us
max            : 2074.8 us
budżet (1/360s): 2777.8 us
max / budżet   : 74.7 %   (MIEŚCI SIĘ)
przepustowość  : 759 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1359.8 us
p99            : 1708.6 us
p99,9          : 1834.8 us
max            : 2176.0 us
max / budżet   : 78.3 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 20.6 us
p99            : 26.0 us
p99,9          : 31.3 us
max            : 219.3 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.61 mem_used_kb=298869 temp_mC=37347
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
