# K24d / H10 — werdykt

> **Odniesienie bramki zaktualizowane 2026-08-18 (K24/H10, faza 3).** Trzy klasy
> przeszły z reżimu zawyżającego do dokładnego: `SUB`, `THETA`, `NTHETA`. Bramka
> zgłosiła POPRAWĘ na obu ziarnach, a plik odniesienia jest aktualizowany w tym
> samym commicie co zmiana silnika — inaczej regresja z powrotem do zawyżania
> nie byłaby wykrywana (README.md, „Gdy bramka oblewa”).
>
> Ten plik jest **odniesieniem regresyjnym**, nie werdyktem kampanii. Werdyktem
> obowiązującym dla artykułu pozostaje K24d (`rdb-experiment/results_20260807_K24d/`,
> silnik `34db1a2`) do czasu kampanii K24e z własną predeklaracją i własnym
> ziarnem. Liczby poniżej pochodzą z przebiegu bramki na drzewie roboczym
> `0f273d5` + zmiana fazy 3; po commicie SHA w nagłówku będzie inny, co bramki
> nie dotyczy — porównuje ona reżimy, nie rewizje.


Korpus: **10010 planów**, **35544 obserwacji węzłowych**, zero błędów aparatury. Ziarno 20260804, silnik `0f273d5`.

Werdykt jest raportowany per klasa operatora. Zgodność 100% jest jedynym
wsparciem H10a w klasie; jedna niezgodność falsyfikuje H10a w tej klasie.

## 1. H10a — dokładność, per klasa operatora

Kolumna **izolowana** jest werdyktem: postać zamknięta policzona z ogonów
składowych wziętych z oracle'a, więc niezgodność pochodzi z reguły tego
węzła. Kolumna **propagowana** to zgodność zrzutu planu silnika z oracle'em
na całym planie — zawiera skutki niezgodności odziedziczonych po dzieciach.

| Klasa | Węzłów | Izolowana C1 | Izolowana C2 | Propagowana C1 | Reżim | Werdykt H10a |
|---|---:|---:|---:|---:|---|---|
| `HASH` | 5960 | 100.0% | 50.0% | 100.0% | dokładna | **wsparta** |
| `SHIFT` | 5314 | 100.0% | 95.6% | 100.0% | dokładna | **wsparta** |
| `PASS` | 4825 | 100.0% | 0.0% | 100.0% | dokładna | **wsparta** |
| `SUB` | 4329 | 100.0% | 69.7% | 100.0% | dokładna | **wsparta** |
| `AGSE` | 4256 | 100.0% | 43.7% | 100.0% | dokładna | **wsparta** |
| `REDUCE` | 3292 | 100.0% | 0.0% | 100.0% | dokładna | **wsparta** |
| `THETA` | 2578 | 100.0% | 67.8% | 100.0% | dokładna | **wsparta** |
| `NTHETA` | 2503 | 100.0% | 99.2% | 100.0% | dokładna | **wsparta** |
| `ADD` | 2487 | 100.0% | 42.9% | 100.0% | dokładna | **wsparta** |

### Trzy reżimy

* **dokładna** (postać zamknięta == oracle wszędzie): `HASH`, `SHIFT`, `PASS`, `SUB`, `AGSE`, `REDUCE`, `THETA`, `NTHETA`, `ADD`;
* **zawyżająca** (nigdy nie zaniża, bezpieczna, ale nie równa): brak;
* **zaniżająca** (ogon mniejszy od wymaganego przez model zdarzeniowy): brak.

Reżim zaniżający jest jakościowo inny od zawyżającego: zawyżenie
opóźnia emisję o slot, zaniżenie oznacza rekord wyemitowany, zanim
wszystkie jego zależności są określone.

### Rozkład różnicy (postać zamknięta − oracle C1)

| Klasa | Rozkład |
|---|---|
| `HASH` | `+0`: 5960 (100.0%) |
| `SHIFT` | `+0`: 5314 (100.0%) |
| `PASS` | `+0`: 4825 (100.0%) |
| `SUB` | `+0`: 4329 (100.0%) |
| `AGSE` | `+0`: 4256 (100.0%) |
| `REDUCE` | `+0`: 3292 (100.0%) |
| `THETA` | `+0`: 2578 (100.0%) |
| `NTHETA` | `+0`: 2503 (100.0%) |
| `ADD` | `+0`: 2487 (100.0%) |

### Świadkowie

| Klasa | Kierunek | Plan | Węzeł | Interwał | Silnik | Postać zamknięta (izol.) | Oracle C1 |
|---|---|---:|---|---|---:|---:|---:|

## 1b. H10a — początek logiczny, per klasa operatora

Wielkość wprowadzona przestemplowaniem z 2026-08-06 i nieobecna
w kampaniach K24/K24r. Kolumna **suma** porównuje origin+ogon —
to jedyna wielkość wspólna z kampaniami sprzed zmiany.

| Klasa | Węzłów | Izolowana | Propagowana | Suma (origin+ogon) | Reżim | Werdykt |
|---|---:|---:|---:|---:|---|---|
| `HASH` | 5960 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `SHIFT` | 5314 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `PASS` | 4825 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `SUB` | 4329 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `AGSE` | 4256 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `REDUCE` | 3292 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `THETA` | 2578 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `NTHETA` | 2503 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |
| `ADD` | 2487 | 100.0% | 100.0% | 100.0% | dokładna | **wsparta** |

### Rozkład różnicy origin (rachunek silnika − oracle)

| Klasa | Rozkład |
|---|---|
| `HASH` | `+0`: 5960 (100.0%) |
| `SHIFT` | `+0`: 5314 (100.0%) |
| `PASS` | `+0`: 4825 (100.0%) |
| `SUB` | `+0`: 4329 (100.0%) |
| `AGSE` | `+0`: 4256 (100.0%) |
| `REDUCE` | `+0`: 3292 (100.0%) |
| `THETA` | `+0`: 2578 (100.0%) |
| `NTHETA` | `+0`: 2503 (100.0%) |
| `ADD` | `+0`: 2487 (100.0%) |

Origin zaniżony (odczyt przed początkiem źródła): **brak**.

## 2. H10b — nielokalność

* rozjazd reguły lokalnej A z dokładną: **5250 z 10010 planów = 52.4%** (próg predeklarowany: >= 5%)
* populacja predeklarowana (dokładnie jeden `#`, poza tym `PASS`/`>N`): **515 planów**, rozjazdów dodatnich **353**
* rozjazdów o predeklarowanej postaci `ceil((p+q-1)/p)`: **353 z 353** (100.0%; próg: 100%)

## 3. Kontrole negatywne

| Kontrola | Węzłów | Rozjazdów | Stan |
|---|---:|---:|---|
| HC_SINGLE (dosłownie) | 3929 | 0 | **przeszła** |
| HC_SINGLE (operatory bez własnego ogona) | 3645 | 0 | **przeszła** |
| HC_INT (dosłownie) | 6821 | 3168 | **ZŁAMANA** |
| HC_INT (węzły `#`, reguła lokalna B) | 2859 | 346 | **ZŁAMANA** |

Obie kontrole predeklarowane **w postaci dosłownej są złamane**.
Zgodnie z kryterium §6 oznacza to źle zdefiniowaną regułę
lokalną, a nie wynik — dlatego **człon (b) jest nieocenialny na tej
aparaturze** i powyższe liczby H10b nie stanowią werdyktu. Diagnoza
sprzeczności w specyfikacji członu (b): REPORT.md §5.

