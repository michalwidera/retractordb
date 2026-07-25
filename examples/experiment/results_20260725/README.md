# Eksperyment: równoważność i koszt reprezentacji przeplotu (2026-07-25)

Cel: sprawdzić, czy operator przeplotu `#` implementowany przez RetractorDB
produkuje ten sam obserwowalny ciąg co klasyczne reprezentacje dataflow —
cyklostatyczna (CSDF) i blokowa (SDF) — oraz wyznaczyć koszt strukturalny
każdej z tych reprezentacji.

Eksperyment jest kontrolą **poprawności i reprezentacji**, nie kampanią
wydajnościową. Nie mierzy czasu i nie służy do porównań przepustowości.

## Dlaczego oracle jest zdefiniowany inaczej niż silnik

Istniejące prototypy w `examples/python-model/` przepisują ten sam wzór
podłogowy, którego używa silnik. Wykrywają literówkę w transkrypcji do C++,
ale nie wykryją błędu w samym wzorze ani w regule remisów: popełniłyby ten
sam błąd.

Dlatego `reference.py` definiuje przeplot **operacyjnie**, przez scalenie
dwóch arytmetycznych siatek terminów:

```
termin A[k] = (k+1) · Δa
termin B[j] =  j    · Δb
scalanie rosnące, remis rozstrzygany na korzyść A
```

Nie ma tu żadnej podłogi ani sufitu. Zgodność z formułą Beatty'ego jest więc
zgodnością dwóch niezależnych konstrukcji. Przesunięcie o jeden interwał po
stronie A odpowiada jednoslotowemu opóźnieniu `Theta` z dokumentacji algebry.

Reguła remisu jest **parametrem** (`tie_to_a`), a nie decyzją wbudowaną: to
jedyne miejsce, w którym dwie poprawne skądinąd implementacje mogą się
rozejść, więc musi dać się je przetestować w obie strony.

## Porównywane realizacje

| model | opis |
|---|---|
| `beatty_online` | wzór podłogowy używany przez silnik |
| `csdf_explicit` | aktor CSDF o `P` jawnych fazach; tablica faz z oracle'a |
| `csdf_lookup` | ten sam aktor, tablica faz ze wzoru Beatty'ego |
| `sdf_block` | aktor blokowy: `b/g` tokenów z A, `a/g` z B, blok `P` tokenów |

Obserwowalnym śladem każdej realizacji jest ciąg par
`(source-id, source-index)`. Dzięki indeksowi źródła test nie może uznać
dwóch równych wartości za tę samą próbkę.

## Hipotezy

- **H1** — wszystkie cztery realizacje dają ten sam ślad co oracle.
- **H2** — minimalny okres słowa wyboru wynosi `P = (a+b)/g` dla
  `Δa/Δb = a/b`, `g = gcd(a,b)`, a w jednym okresie występuje `b/g` tokenów
  z A i `a/g` z B.
- **H3** — jawny CSDF przechowuje `Θ(P)` faz, podczas gdy realizacja online
  używa stałej liczby liczników; aktor blokowy SDF ma zwartą sygnaturę
  rate'ów, ale jego startup latency i bufor wejściowy rosną z `P`.
- **H4** — artefakt binarny produkowany przez `xretractor` zgadza się z
  oracle'em rekord po rekordzie.

## Struktura

```
reference.py         niezależny oracle (scalanie terminów) + lemat o okresie
models.py            cztery realizacje + koszty strukturalne reprezentacji
test_equivalence.py  kontrole mutacyjne + kampanie porównawcze
engine_check.py      most: artefakt xretractor kontra ślad oracle'a
make_summary.py      generuje results/summary.md z surowych JSON-ów
run.sh               odtwarza całość jednym poleceniem
results/             wyniki (summary.md, equivalence.json, engine.json, raw/)
work/                katalogi robocze mostu do silnika (generowane)
```

## Odtworzenie

```bash
./run.sh                                  # pełny przebieg
QUICK=1 ./run.sh                          # skrócona macierz
XRETRACTOR=/ścieżka/do/xretractor ./run.sh
```

Most do silnika wymaga zbudowanej binarki; domyślnie szuka
`build/Debug/src/retractor/xretractor`, a w razie braku — `xretractor`
w `PATH`.

## Kontrole mutacyjne

Macierz zaczyna się od wstrzyknięcia błędów. Test, który nie zawodzi po
mutacji, nie jest kontrolą, tylko dekoracją.

| mutacja | co psuje |
|---|---|
| `beatty_off_by_one` | decyzja przesunięta o jeden slot |
| `beatty_a_index` | poprawne słowo wyboru, błędny indeks źródłowy A |
| `oracle_tie_to_b` | odwrócona reguła remisu w definicji odniesienia |
| `csdf_period_minus_one` | poprawna tablica faz, moduł o jeden za mały |
| `unreduced_ratio_is_benign` | **kontrola negatywna** — nie powinna nic zmienić |

Ostatnia pozycja pilnuje drugiej strony: okres `a+b` bez skrócenia przez `g`
jest reprezentacją poprawną, tylko niemninimalną, więc macierz nie ma prawa
zgłosić tu rozbieżności.

### Odrzucona mutacja

Pierwotnie mutowano indeks po stronie B (`n − ⌊nz⌋` zamiast `n − ⌊(n+1)z⌋`).
Okazało się to tożsamością: w gałęzi B warunek rozgałęzienia brzmi dokładnie
`⌊nz⌋ = ⌊(n+1)z⌋`, więc obie postaci są równe. Mutacja została przeniesiona
na stronę A, gdzie `⌊(n+1)z⌋ = ⌊nz⌋ + 1` i podmiana daje prawdziwy
off-by-one. Zapisane, bo jest to przykład kontroli, która wyglądała
sensownie, a niczego nie sprawdzała.

## Ograniczenia mostu do silnika

- Źródło plikowe czytane poza koniec danych zaczyna indeksować od zera.
  Porównywany jest wyłącznie prefiks pokryty wygenerowanymi danymi; liczba
  faktycznie porównanych rekordów jest raportowana w kolumnie `porównano`.
- Silnik taktuje w czasie rzeczywistym, więc długość przebiegu ogranicza
  liczbę rekordów możliwych do zebrania w rozsądnym czasie, a nie moc
  obliczeniowa.
- Interwały podawane są w RQL jako ułamki `p/q`, co omija ścieżkę
  `FLOAT → stod → Rationalize` i pozwala badać semantykę, a nie parser.

## Czego ten eksperyment nie robi

- Nie mierzy czasu, przepustowości ani `ns/token`.
- Nie twierdzi, że SDF/CSDF nie potrafią wyrazić przeplotu. Aktor SDF może
  zawierać w sobie ten sam algorytm; porównywane są jawne, klasyczne
  reprezentacje rate/phase z reprezentacją algorytmiczną.
- Nie zastępuje recenzji dowodu formalnego. Jest kontrolą maszynową
  o zadeklarowanym zakresie.

## Wyniki

Przebieg z 2026-07-25 na silniku `e189d0e`:

- kontrole mutacyjne: 4/4 wstrzyknięte błędy wykryte, kontrola negatywna
  poprawnie milczy;
- macierz równoważności: 7908 par interwałów, 4 106 738 porównanych pozycji,
  **zero rozbieżności** i zero błędów okresu;
- most do silnika: **8/8** przypadków zgodnych rekord po rekordzie, łącznie ze
  stosunkiem 160/147 (`P = 307`, 1151 rekordów, 3,75 okresu);
- koszt reprezentacji: jawny CSDF przechowuje `P` faz przy startup latency 1
  tokena; blokowy SDF ma zwarty opis rate'ów, ale startup latency i bufor
  wejściowy równe `P`; postać online utrzymuje stałą liczbę liczników.

Obserwacja uboczna: dla samego `A#B`, bez operatorów przesunięcia, artefakt
nie ma zerowego prefiksu w żadnym z ośmiu przypadków.

Pełne tabele: [`results/summary.md`](results/summary.md) — plik jest
generowany przez `run.sh` i nie należy go edytować ręcznie.
