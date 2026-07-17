# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T12:38:33+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /tmp/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 7290.6 us
p99            : 8884.6 us
max            : 14488.0 us
budżet (1/360s): 2777.8 us
max / budżet   : 521.6 %   (PRZEKROCZONY)
przepustowość  : 137 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 106777542.9 us
p99            : 211784608.2 us
p99,9          : 213717053.6 us
max            : 213921782.8 us
max / budżet   : 7701184.2 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 106766612.1 us
p99            : 211771733.3 us
p99,9          : 213707372.3 us
max            : 213909637.2 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.83 mem_used_kb=264831 temp_mC=39646
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
