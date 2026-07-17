# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T19:30:51+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 2006.2 us
p99            : 2501.1 us
max            : 6610.8 us
budżet (1/720s): 1388.9 us
max / budżet   : 476.0 %   (PRZEKROCZONY)
przepustowość  : 493 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 32633323.4 us
p99            : 64927584.8 us
p99,9          : 65518812.6 us
max            : 65581860.8 us
max / budżet   : 4721894.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 32629672.4 us
p99            : 64923792.5 us
p99,9          : 65515169.9 us
max            : 65578291.7 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.53 mem_used_kb=269496 temp_mC=41086
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
