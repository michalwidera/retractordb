# Wyniki badania 4 -- kampania rate_fine

- data: 2026-07-22T00:12:26+02:00
- czestosc napływu: 480 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_4/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1354.4 us
p99            : 1623.2 us
max            : 1824.9 us
budżet (1/480s): 2083.3 us
max / budżet   : 87.6 %   (MIEŚCI SIĘ)
przepustowość  : 731 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1417.6 us
p99            : 1721.9 us
p99,9          : 1787.5 us
max            : 1928.8 us
max / budżet   : 92.6 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 23.7 us
p99            : 28.7 us
p99,9          : 32.2 us
max            : 37.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.90 mem_used_kb=289081 temp_mC=39750
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
