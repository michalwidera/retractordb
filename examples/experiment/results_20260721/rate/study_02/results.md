# Wyniki badania 2 -- kampania rate

- data: 2026-07-21T20:06:26+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1308.0 us
p99            : 1556.2 us
max            : 43634.9 us
budżet (1/720s): 1388.9 us
max / budżet   : 3141.7 %   (PRZEKROCZONY)
przepustowość  : 735 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 363122.9 us
p99            : 728864.4 us
p99,9          : 733064.8 us
max            : 734003.8 us
max / budżet   : 52848.3 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 361756.0 us
p99            : 727506.1 us
p99,9          : 731671.8 us
max            : 732671.9 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.44 mem_used_kb=285529 temp_mC=38268
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
