# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T16:42:57+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 3797.1 us
p99            : 4524.9 us
max            : 6810.1 us
budżet (1/720s): 1388.9 us
max / budżet   : 490.3 %   (PRZEKROCZONY)
przepustowość  : 259 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 64047796.5 us
p99            : 127008283.2 us
p99,9          : 128174232.9 us
max            : 128296201.2 us
max / budżet   : 9237326.5 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 64041669.1 us
p99            : 127001876.8 us
p99,9          : 128167304.5 us
max            : 128289890.4 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.04 mem_used_kb=268605 temp_mC=40848
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
