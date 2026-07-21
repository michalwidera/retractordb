# Wyniki badania S3 (mlock-variant) -- sledztwo 40ms, Faza 2

- data: 2026-07-18T14:24:06+02:00
- silnik: rdzen 3, -t (FIFO 50), 360 Hz, 5000 probek, klient xqry po 2 s
- warianty RDB_MLOCKALL: populate onfault off, po 3 przebiegow
- hipoteza: spike e2e ~42 ms przy attach znika w 'onfault' i 'off'

## uname
```
Linux pi400 6.8.0-2049-raspi-realtime #50-Ubuntu SMP PREEMPT_RT Mon Jun 29 16:03:02 UTC 2026 aarch64 aarch64 aarch64 GNU/Linux
```

## Analiza (spike e2e przy attach; zdarzenia wake >= 5 ms po slocie 100)

| wariant | rep | max e2e po slocie 100 [ms] (slot) | zdarzen wake>=5ms po slocie 100 | szczyt serii [ms] |
|---|---|---|---|---|
| populate | 1 | 42.489 (706) | 47 | 40.238 |
| populate | 2 | 42.056 (708) | 44 | 39.108 |
| populate | 3 | 42.027 (708) | 45 | 39.078 |
| onfault | 1 | 17.403 (708) | 16 | 14.451 |
| onfault | 2 | 17.314 (708) | 14 | 14.370 |
| onfault | 3 | 17.297 (708) | 14 | 14.345 |
| off | 1 | 17.306 (708) | 14 | 14.353 |
| off | 2 | 17.306 (708) | 14 | 14.351 |
| off | 3 | 17.307 (708) | 14 | 14.354 |

## Polityki watkow silnika (przyklad z ostatniego przebiegu; FF=FIFO, TS=CFS)
```
   2209    2209 xretractor      FF      50   3
   2209    2210 xretractor      TS       -   3
```
## Segment kolejki klienta w /dev/shm
```
-rw-r--r-- 1 michal michal 3773016 Jul 18 14:26 /dev/shm/brcdbr2211
```
