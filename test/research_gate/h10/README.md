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
| `tests/test_phase_forms.py` | ogon `-`, `Θ` i `~Θ` wobec modelu zdarzeniowego dla `q` do 12 — poza zakresem korpusu losowego (`q <= 5`); ma własną kontrolę mocy detekcyjnej |
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

## Poprawka reguły lokalnej A z 2026-09-12 — `>N` skraca ogon

Reguła lokalna A liczyła dla `>N` sam ogon składowej przeliczony przez takt,
podczas gdy postać dokładna to `max(0, W_src - N)`. Rozjazd miał postać zamkniętą
`-min(N, W_src)` i był **ujemny**: reguła ZAWYŻAŁA ogon. Trafiał 5314/5314 węzłów
`SHIFT` na ziarnie 20260804 i 5438/5438 na 20260807, czyli co do jednego.

Historia jest pouczająca. Do K24r reguła miała wyjątek `+N`; usunięto go słusznie,
bo po przestemplowaniu z 2026-08-06 `N` przeszło do `logicalOrigin` i ogonem nie
jest. Usunięcie zostawiło jednak **brak członu** tam, gdzie prawda brzmi `-N` —
reguła przeszła z zaniżania na zawyżanie zamiast trafić.

**To nie był defekt silnika.** Na wszystkich tych węzłach `engine_tail == oracle_c1`;
rozjazd żył wyłącznie między regułą diagnostyczną a postacią dokładną.

Znaczenie dla członu (b) jest metodologiczne: `max(0, W_src - N)` zależy wyłącznie
od ogona dziecka i od WŁASNEGO parametru węzła, więc jest w pełni **lokalna**.
Reguła bez tego członu mierzyła własny brak, a nie nielokalność — była chochołem
dla operatorów niosących parametr.

Co ta poprawka zmienia, a czego nie:

| wielkość | przed | po |
|---|---:|---:|
| HC_INT (dosłownie), ziarno 20260804 | 3168 | **2999** |
| HC_INT (dosłownie), ziarno 20260807 | 3226 | **3039** |
| HC_INT (węzły `#`, reguła lokalna B) | 346 / 340 | bez zmian |
| HC_SINGLE, obie postaci | 0 | bez zmian |
| człon (b): odsetek planów z rozjazdem | 52,4% / 52,7% | bez zmian |
| człon (b): rozjazdy o predeklarowanej postaci | 353/353, 358/358 | bez zmian |
| reżimy H10a (ogon i początek logiczny) | 9/9 dokładnych | bez zmian |

Nagłówkowa liczba członu (b) jest odporna, bo **żaden** plan nie rozjeżdżał się
wyłącznie na `SHIFT` — każdy rozjeżdżający się plan rozjeżdża się też na innym
rodzaju węzła. Poprawka czyści kontrolę negatywną, a nie wynik.

**`HC_INT` pozostaje ZŁAMANA** (2999 i 3039 rozjazdów), więc człon (b) nadal jest
NIEOCENIALNY. Ta poprawka jest warunkiem koniecznym oceny, nie wystarczającym:
wąskim gardłem zostaje człon pierwszej fazy `#`, o dokładnie jeden slot za krótki
w 346 z 2859 węzłów o ilorazie całkowitym (rozkład rozjazdu to wyłącznie `{0, 1}`).

**Pliki odniesienia zostają nietknięte.** `VERDICT.md` i `VERDICT_oos.md` są zapisem
kampanii, która szła ze starą regułą, i mają nim pozostać — świeży werdykt różni się
od nich w sekcji 3 dokładnie o wielkości z tabeli powyżej i **nie jest to dryft**.
Jądro bramki (`compare_regimes.py`) czyta wyłącznie dwie tabele reżimów H10a, więc
tej różnicy nie widzi i widzieć nie ma. Przeliczenie członu (b) poprawioną regułą
wymaga własnej predeklaracji i osobnego przebiegu; dopasowanie kryterium do danych
po fakcie unieważniłoby wynik.

## Defekt aparatury z 2026-08-19 — binarka nie może być zgadywana

Poziom `test_closedform` oblał na CI z powodu, który **nie miał nic wspólnego
z silnikiem**: `run_gate.sh` jako jedynemu testowi rozmawiającemu z silnikiem
nie podawał ścieżki do binarki. `oracle/engine.py` miał wtedy dwa domyślne
zachowania i oba były pułapkami:

1. `DEFAULT_BINARY` wskazywała `parents[3]/retractordb/build/Debug/...`, czyli
   układ katalogów **repozytorium eksperymentu**. Po przeniesieniu aparatury do
   drzewa silnika ta ścieżka nie istnieje i nigdy nie zadziałała.
2. Po niej wchodził fallback na `xretractor` z `PATH`. Na CI nie ma tam nic
   (job bramki nie robi `ninja install`), więc poziom oblewał. Lokalnie stoi tam
   binarka **zainstalowana**, więc poziom przechodził — sprawdzając inną binarkę
   niż cała reszta bramki.

Drugi przypadek jest groźniejszy od pierwszego: czerwone światło widać, a ciche
podstawienie binarki wygląda jak dowód. Poprawka jest w dwóch miejscach:
`run_gate.sh` podaje `$XRETRACTOR` jawnie każdemu wywołaniu, a
`engine.resolve_binary` nie ma już ani ścieżki domyślnej, ani fallbacku na
`PATH` — bez jawnego wskazania podnosi `EngineError`. Wołający zawsze wie, którą
binarkę bada.

Zanim to naprawiono, test uruchomiono ręcznie na binarce z `build/Release`:
57 porównanych węzłów, wierność repliki potwierdzona. Mechanizm był nietknięty,
czerwone światło pochodziło wyłącznie z aparatury.
