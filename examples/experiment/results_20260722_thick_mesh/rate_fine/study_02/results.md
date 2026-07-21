# Wyniki badania 2 -- kampania rate_fine

- data: 2026-07-22T00:08:33+02:00
- czestosc napływu: 420 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1323.9 us
p99            : 1605.9 us
max            : 1851.0 us
budżet (1/420s): 2381.0 us
max / budżet   : 77.7 %   (MIEŚCI SIĘ)
przepustowość  : 748 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1385.8 us
p99            : 1704.3 us
p99,9          : 1754.3 us
max            : 1984.3 us
max / budżet   : 83.3 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 21.8 us
p99            : 28.7 us
p99,9          : 32.7 us
max            : 61.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.47 mem_used_kb=291529 temp_mC=38935
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
