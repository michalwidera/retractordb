# Wyniki badania S1 (pacer-solo) -- sledztwo 40ms, Faza 0

- data: 2026-07-18T12:08:08+02:00
- pacer: rdzen 3 (isolcpus), FIFO 50, 360 Hz, 200000 slotow, zero pracy w slocie
- pytanie: czy platforma BEZ silnika generuje zdarzenia >= 5 ms?

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| pacer-solo | 200000 | 22.5 | 23.4 | 0.025 | 0.057 | 0 |

**pacer-solo**: zero zdarzen >= progu.


### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- pacer-solo czysty -> platforma sama z siebie nie generuje zdarzen >= progu na rdzeniu 3 (spojne z czystym baselinem NumPy); ciezar przesuwa sie na H2.

## Log pacera
```
PID 2074
shadow_pacer[S1-core3]: sloty=200000 mediana=22.5us max=0.057ms
```
