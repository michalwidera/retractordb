# Wyniki badania S2 (engine-shadow) -- sledztwo 40ms, Faza 0

- data: 2026-07-18T14:50:16+02:00
- silnik: rdzen 3, FIFO 50 (-t), 360 Hz, 20000 probek, 1 klient xqry (sink=null, rdzenie 0-1)
- cien: shadow_pacer.py na rdzeniu 2, FIFO 50, zero pracy
- powtorzen: 5; miedzy powtorzeniami bez rebootu (reboot rozdziela badania, nie powtorzenia)
- pytanie: czy zdarzenia >= 5 ms u silnika widzi rownoczesnie niezalezny proces na sasiednim rdzeniu?

## Powtorzenie 1

- launch_mono_ns=2713700234354 exit_mono_ns=2769410678173

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.5 | 90.9 | 0.525 | 0.610 | 0 |
| shadow | 19382 | 26.5 | 32.7 | 0.042 | 0.181 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.16 s => tolerancja dopasowania +/-0.18 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    340870     361288      43226     120917       Rescheduling interrupts
< IPI1:      1659     143733      50377       1236       Function call interrupts
> IPI0:    387100     407726      43815     137090       Rescheduling interrupts
> IPI1:      1671     145706      52440       1279       Function call interrupts
< IPI5:    131689      15310      16420        190       IRQ work interrupts
> IPI5:    131739      15328      16443        209       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 2

- launch_mono_ns=2770069050943 exit_mono_ns=2825772820257

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.7 | 68.8 | 0.518 | 0.543 | 0 |
| shadow | 19381 | 26.5 | 30.8 | 0.039 | 0.092 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.15 s => tolerancja dopasowania +/-0.18 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    387126     407776      43971     137090       Rescheduling interrupts
< IPI1:      1675     145769      52497       1279       Function call interrupts
> IPI0:    433213     454134      44405     153331       Rescheduling interrupts
> IPI1:      1689     147689      54547       1320       Function call interrupts
< IPI5:    131740      15328      16443        209       IRQ work interrupts
> IPI5:    131792      15339      16453        226       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 3

- launch_mono_ns=2826432727121 exit_mono_ns=2882140259509

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 23.0 | 51.0 | 0.496 | 0.522 | 0 |
| shadow | 19382 | 27.6 | 31.6 | 0.053 | 0.222 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.15 s => tolerancja dopasowania +/-0.18 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    433240     454283      44453     153331       Rescheduling interrupts
< IPI1:      1691     147732      54562       1320       Function call interrupts
> IPI0:    478937     499997      44768     169534       Rescheduling interrupts
> IPI1:      1702     149623      56494       1363       Function call interrupts
< IPI5:    131794      15339      16453        226       IRQ work interrupts
> IPI5:    131848      15352      16459        245       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 4

- launch_mono_ns=2882797842025 exit_mono_ns=2938504460732

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 23.0 | 62.0 | 0.503 | 0.541 | 0 |
| shadow | 19381 | 27.2 | 37.8 | 0.053 | 0.104 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.15 s => tolerancja dopasowania +/-0.18 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    478959     500028      44800     169534       Rescheduling interrupts
< IPI1:      1704     149697      56503       1363       Function call interrupts
> IPI0:    524853     546004      45216     185757       Rescheduling interrupts
> IPI1:      1715     152794      58611       1404       Function call interrupts
< IPI5:    131849      15352      16459        245       IRQ work interrupts
> IPI5:    131901      15366      16462        265       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 5

- launch_mono_ns=2939163191322 exit_mono_ns=2994870202956

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.6 | 57.7 | 0.498 | 0.534 | 0 |
| shadow | 19381 | 27.5 | 31.1 | 0.050 | 0.091 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.15 s => tolerancja dopasowania +/-0.18 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    524878     546151      45280     185757       Rescheduling interrupts
< IPI1:      1717     152826      58620       1404       Function call interrupts
> IPI0:    571096     592342      45560     201777       Rescheduling interrupts
> IPI1:      1725     154665      61069       1446       Function call interrupts
< IPI5:    131901      15366      16462        265       IRQ work interrupts
> IPI5:    131960      15379      16473        280       IRQ work interrupts
(brak roznic w liniach IPI)
```
