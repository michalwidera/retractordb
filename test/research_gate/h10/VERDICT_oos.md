# K24d / H10 — werdykt

Korpus: **10010 planów**, **35582 obserwacji węzłowych**, zero błędów aparatury. Ziarno 20260807, silnik `34db1a2`.

Werdykt jest raportowany per klasa operatora. Zgodność 100% jest jedynym
wsparciem H10a w klasie; jedna niezgodność falsyfikuje H10a w tej klasie.

## 1. H10a — dokładność, per klasa operatora

Kolumna **izolowana** jest werdyktem: postać zamknięta policzona z ogonów
składowych wziętych z oracle'a, więc niezgodność pochodzi z reguły tego
węzła. Kolumna **propagowana** to zgodność zrzutu planu silnika z oracle'em
na całym planie — zawiera skutki niezgodności odziedziczonych po dzieciach.

| Klasa | Węzłów | Izolowana C1 | Izolowana C2 | Propagowana C1 | Reżim | Werdykt H10a |
|---|---:|---:|---:|---:|---|---|
| `HASH` | 5998 | 100.0% | 49.6% | 93.7% | dokładna | **wsparta** |
| `SHIFT` | 5438 | 100.0% | 95.8% | 99.7% | dokładna | **wsparta** |
| `PASS` | 4668 | 100.0% | 0.0% | 97.3% | dokładna | **wsparta** |
| `SUB` | 4320 | 19.3% | 36.8% | 6.8% | zawyżająca | **FALSYFIKACJA** |
| `AGSE` | 4242 | 100.0% | 43.9% | 98.0% | dokładna | **wsparta** |
| `REDUCE` | 3237 | 100.0% | 0.0% | 98.9% | dokładna | **wsparta** |
| `NTHETA` | 2619 | 99.4% | 99.4% | 98.2% | zawyżająca | **FALSYFIKACJA** |
| `THETA` | 2612 | 60.7% | 88.4% | 53.8% | zawyżająca | **FALSYFIKACJA** |
| `ADD` | 2448 | 100.0% | 44.3% | 97.5% | dokładna | **wsparta** |

### Trzy reżimy

* **dokładna** (postać zamknięta == oracle wszędzie): `HASH`, `SHIFT`, `PASS`, `AGSE`, `REDUCE`, `ADD`;
* **zawyżająca** (nigdy nie zaniża, bezpieczna, ale nie równa): `SUB`, `NTHETA`, `THETA`;
* **zaniżająca** (ogon mniejszy od wymaganego przez model zdarzeniowy): brak.

Reżim zaniżający jest jakościowo inny od zawyżającego: zawyżenie
opóźnia emisję o slot, zaniżenie oznacza rekord wyemitowany, zanim
wszystkie jego zależności są określone.

### Rozkład różnicy (postać zamknięta − oracle C1)

| Klasa | Rozkład |
|---|---|
| `HASH` | `+0`: 5998 (100.0%) |
| `SHIFT` | `+0`: 5438 (100.0%) |
| `PASS` | `+0`: 4668 (100.0%) |
| `SUB` | `+0`: 834 (19.3%), `+1`: 3486 (80.7%) |
| `AGSE` | `+0`: 4242 (100.0%) |
| `REDUCE` | `+0`: 3237 (100.0%) |
| `NTHETA` | `+0`: 2603 (99.4%), `+1`: 16 (0.6%) |
| `THETA` | `+0`: 1585 (60.7%), `+1`: 1027 (39.3%) |
| `ADD` | `+0`: 2448 (100.0%) |

### Świadkowie

| Klasa | Kierunek | Plan | Węzeł | Interwał | Silnik | Postać zamknięta (izol.) | Oracle C1 |
|---|---|---:|---|---|---:|---:|---:|
| `SUB` | zawyżenie | 4 | n0 | `1/4` | 1 | 1 | 0 |
| `SUB` | zawyżenie | 11 | n0 | `1` | 1 | 1 | 0 |
| `NTHETA` | zawyżenie | 83 | n5 | `3/5` | 1 | 1 | 0 |
| `NTHETA` | zawyżenie | 615 | n3 | `1` | 1 | 1 | 0 |
| `THETA` | zawyżenie | 5 | n2 | `35/8` | 2 | 2 | 1 |
| `THETA` | zawyżenie | 5 | n3 | `175/24` | 3 | 2 | 1 |

## 1b. H10a — początek logiczny, per klasa operatora

Wielkość wprowadzona przestemplowaniem z 2026-08-06 i nieobecna
w kampaniach K24/K24r. Kolumna **suma** porównuje origin+ogon —
to jedyna wielkość wspólna z kampaniami sprzed zmiany.

| Klasa | Węzłów | Izolowana | Propagowana | Suma (origin+ogon) | Reżim | Werdykt |
|---|---:|---:|---:|---:|---|---|
| `HASH` | 5998 | 100.0% | 100.0% | 93.7% | dokładna | **wsparta** |
| `SHIFT` | 5438 | 100.0% | 100.0% | 99.7% | dokładna | **wsparta** |
| `PASS` | 4668 | 100.0% | 100.0% | 97.3% | dokładna | **wsparta** |
| `SUB` | 4320 | 100.0% | 100.0% | 6.8% | dokładna | **wsparta** |
| `AGSE` | 4242 | 100.0% | 100.0% | 98.0% | dokładna | **wsparta** |
| `REDUCE` | 3237 | 100.0% | 100.0% | 98.9% | dokładna | **wsparta** |
| `NTHETA` | 2619 | 100.0% | 100.0% | 98.2% | dokładna | **wsparta** |
| `THETA` | 2612 | 100.0% | 100.0% | 53.8% | dokładna | **wsparta** |
| `ADD` | 2448 | 100.0% | 100.0% | 97.5% | dokładna | **wsparta** |

### Rozkład różnicy origin (rachunek silnika − oracle)

| Klasa | Rozkład |
|---|---|
| `HASH` | `+0`: 5998 (100.0%) |
| `SHIFT` | `+0`: 5438 (100.0%) |
| `PASS` | `+0`: 4668 (100.0%) |
| `SUB` | `+0`: 4320 (100.0%) |
| `AGSE` | `+0`: 4242 (100.0%) |
| `REDUCE` | `+0`: 3237 (100.0%) |
| `NTHETA` | `+0`: 2619 (100.0%) |
| `THETA` | `+0`: 2612 (100.0%) |
| `ADD` | `+0`: 2448 (100.0%) |

Origin zaniżony (odczyt przed początkiem źródła): **brak**.

## 2. H10b — nielokalność

* rozjazd reguły lokalnej A z dokładną: **5275 z 10010 planów = 52.7%** (próg predeklarowany: >= 5%)
* populacja predeklarowana (dokładnie jeden `#`, poza tym `PASS`/`>N`): **514 planów**, rozjazdów dodatnich **358**
* rozjazdów o predeklarowanej postaci `ceil((p+q-1)/p)`: **358 z 358** (100.0%; próg: 100%)

## 3. Kontrole negatywne

| Kontrola | Węzłów | Rozjazdów | Stan |
|---|---:|---:|---|
| HC_SINGLE (dosłownie) | 3982 | 0 | **przeszła** |
| HC_SINGLE (operatory bez własnego ogona) | 3691 | 0 | **przeszła** |
| HC_INT (dosłownie) | 6825 | 3226 | **ZŁAMANA** |
| HC_INT (węzły `#`, reguła lokalna B) | 2864 | 340 | **ZŁAMANA** |

Obie kontrole predeklarowane **w postaci dosłownej są złamane**.
Zgodnie z kryterium §6 oznacza to źle zdefiniowaną regułę
lokalną, a nie wynik — dlatego **człon (b) jest nieocenialny na tej
aparaturze** i powyższe liczby H10b nie stanowią werdyktu. Diagnoza
sprzeczności w specyfikacji członu (b): REPORT.md §5.

