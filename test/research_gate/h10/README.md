# Bramka H10 — rachunek początku logicznego i ogona startowego

## Hipoteza, której wyniki ta bramka chroni

**H10a.** Początek logiczny i ogon startowy każdego węzła planu dają się
policzyć **statycznie, z samego planu**, bez uruchamiania strumieni — i wynik
tego rachunku zgadza się z modelem zdarzeniowym.

**H10b.** Reguła lokalna wyznaczania ogona jest wystarczająca, to znaczy ogon
węzła zależy wyłącznie od jego bezpośrednich wejść, a nie od dalszej struktury
planu.

Wynik jest **mieszany i taki pozostaje**. Na silniku, który te liczby wytworzył:

| Reżim | Klasy | Znaczenie |
|---|---|---|
| **dokładna** | wszystkie dziewięć | postać zamknięta równa modelowi zdarzeniowemu wszędzie |
| **zawyżająca** | brak | nigdy nie zaniża; kosztuje slot opóźnienia |
| **zaniżająca** | brak | — |

Początek logiczny jest dokładny we **wszystkich dziewięciu** klasach.
H10b pozostaje wsparta w swojej zawężonej populacji.

**Zmiana z 2026-08-18 (K24/H10, faza 3).** Do tego dnia trzy klasy — `SUB`,
`THETA`, `NTHETA` — były zawyżające, i była to zapisana falsyfikacja H10a
w mocnej postaci. Wyprowadzenie postaci dokładnych
(`rdb-experiment/investigation_K24H10/DERIVATION.md`) domknęło je: wszystkie
trzy przeszły do reżimu dokładnego, bramka zgłosiła POPRAWĘ na obu ziarnach.
Dokumentem obowiązującym dla artykułu pozostaje jednak kampania K24d do czasu
przebiegu K24e z własną predeklaracją — bramka jest zabezpieczeniem rozwoju,
nie dowodem.

## Na czym polega bramka

To **nie jest** powtórzenie kampanii badawczej. To zabezpieczenie rozwoju:
kod silnika ma się dalej zmieniać, a bramka pilnuje, żeby zmiana nie cofnęła
osiągniętej dokładności.

| Krok | Co robi |
|---|---|
| `tests/test_independence.py` | dowodzi, że postać zamknięta nie podgląda modelu zdarzeniowego |
| `tests/test_oracle.py` | 44 przypadki ręczne, 220 porównań — model zdarzeniowy zgadza się z policzonym ręcznie |
| `tests/test_mutants.py` | wstrzykuje błędy do modelu; wykrycie 100% jest warunkiem, żeby model cokolwiek orzekał |
| `tests/test_closedform.py` | replika postaci zamkniętej wierna wobec silnika |
| `run_campaign.py` | generuje 10 010 losowych planów, dla każdego czyta ogon i origin ze zrzutu `xretractor -c` i zestawia z modelem zdarzeniowym |
| `verdict.py` | klasyfikuje każdą klasę operatora do jednego z trzech reżimów |
| `../compare_regimes.py` | **jądro bramki** — porównuje reżimy z odniesieniem, kierunkowo |

Kampania idzie na dwóch ziarnach: `20260804` (odniesienie
[`VERDICT.md`](VERDICT.md)) i `20260807` (odniesienie poza próbą,
[`VERDICT_oos.md`](VERDICT_oos.md)). Oba muszą przejść.

## Co zatrzymuje pracę, a co nie

Porządek reżimów, od najgorszego do najlepszego:

```
zaniżająca  <  zawyżająca  <  dokładna
```

| Zmiana | Reakcja bramki |
|---|---|
| `zawyżająca` → `dokładna` | **POPRAWA** — bramka przechodzi. To pożądany kierunek rozwoju |
| `dokładna` → `zawyżająca` | **REGRESJA** — bramka oblewa |
| cokolwiek → `zaniżająca` | **DEFEKT** — bramka oblewa zawsze |
| zmiana zestawu klas | **BŁĄD** — kod 2, bramka nie orzeka |

Reżim zaniżający jest jakościowo inny od zawyżającego: zawyżenie opóźnia
emisję o slot, zaniżenie oznacza **rekord wyemitowany, zanim wszystkie jego
zależności są określone**. Dlatego jest błędem nawet wtedy, gdyby odniesienie
już go zawierało.

Gdy bramka zgłosi POPRAWĘ, wypisze też przypomnienie: artykuł twierdzi sześć
klas dokładnych i to twierdzenie staje się wtedy **zachowawcze**. Poprawa nie
wymaga żadnej reakcji w kodzie — wymaga odnotowania.

## Gdy bramka oblewa

**Nie poprawiaj bramki.** Czerwone światło znaczy, że zmieniła się semantyka
początku logicznego albo ogona startowego. Kolejność działań: ustal, która
zmiana silnika to spowodowała; rozstrzygnij, czy była zamierzona; jeśli tak —
zaktualizuj plik odniesienia w tym samym commicie i odnotuj zmianę. Podmiana
oczekiwanych wartości bez tego zapisu kasuje jedyną ochronę, jaką ten katalog
daje.

Precedens jest realny i świeży: zmiana silnika przestemplowująca okno `@`
i przenosząca `>N` do origin wypchnęła `HASH` i `SHIFT` z reżimu dokładnego do
zawyżającego. Wyszło to dopiero w osobnym przebiegu badawczym tydzień później.
Ta bramka istnieje po to, żeby wyszło przy commicie.
