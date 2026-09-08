# Fizyczna weryfikacja braku dryftu silnika

```bash
ninja test_drift      # z build/Debug lub build/Release
```

Silnik zmienia się dalej, a wyniki H9 i H10 zostały osiągnięte na konkretnej
rewizji. To polecenie **wykonuje silnik** i sprawdza, czy nie odjechał od tamtych
wyników. Nie jest kampanią, nie produkuje wpisu do repozytorium eksperymentu
i nie zastępuje K24d ani K26v3 jako dokumentu obowiązującego dla artykułu.

## Po co to, skoro jest bramka

[`run_gate.sh`](run_gate.sh) porównuje **etykiety**: bierze dwa zamrożone ziarna,
sprowadza wynik do etykiety reżimu per klasa operatora i zestawia ją z tablicą
odniesienia. Trzy rodzaje dryftu są dla niej niewidoczne **z konstrukcji**:

| Dryft | Dlaczego bramka go nie widzi | Stan |
|---|---|---|
| **wartości** | bramka nie sprawdza, **co** silnik policzył — tylko etykiety reżimów i to, czy 21 planów się kompiluje | **czynny**: złe liczby przy nietkniętych reżimach przechodzą na zielono |
| **poza korpusem** | zawsze te same 2 × 10 010 planów z ziaren `20260804` i `20260807` | **czynny**: zmiana zachowania poza nimi nie ma jak się ujawnić |
| **wielkości w etykiecie** | etykieta `zawyżająca` nie mówi, o ile zawyża | **uśpiony**: dziś wszystkie dziewięć klas jest dokładnych, gdzie etykieta jest ciasna (`step1 == n`) |

`run_drift.sh` konfrontuje silnik z czymś **niezależnym od niego**: z modelem
zdarzeniowym (H10) i z niezależnym portem w Apache Flink (H9, wartości).
Do tego losuje ziarno przy każdym uruchomieniu, więc kolejne przebiegi
przemiatają przestrzeń planów zamiast wracać w to samo miejsce.

| | bramka `test_gate` | `test_drift` |
|---|---|---|
| próba | dwa **zamrożone** ziarna | ziarno **losowane w chwili uruchomienia** |
| odniesienie | tablica `VERDICT.md` | model zdarzeniowy i implementacja w Flinku |
| wartości | **nie sprawdza** | 378 porównań wobec Flinka, ≥2000 rekordów każdego wyniku |
| ślad | brak | wpis do `DRIFT_JOURNAL.tsv`, także dla przebiegu z dryftem |

## Pięć poziomów, trzy wartości

Każdy poziom kończy się jednym z: **ZGODNY**, **DRYFT**, **NIESPRAWDZONY**.
Poziom niesprawdzony **nie jest zaliczony** i nigdy nie jest przemilczany;
`--strict` zamienia go w niezaliczenie całego przebiegu.

| Poziom | Z czym konfrontuje silnik | Wymaga |
|---|---|---|
| **H10a** — początek logiczny i ogon startowy | model zdarzeniowy, na świeżo wylosowanej próbie 10 010 planów | `python3`, `xretractor` |
| **H10b** — wystarczalność reguły lokalnej | te same dane, warunkowo: najpierw predeklarowane kontrole negatywne | jw. |
| **H9 mechanizm** — rozpoznawanie wspólnego podplanu | 84 kompilacje na czterech profilach ablacji, 4 odrzucone mutanty | cztery profile `build/K26v3-*` |
| **H9 wartości** — co silnik policzył | niezależny port w Apache Flink: deskryptory, kolejność, wartości, `NULL`-e, luki | JDK 17, Flink 2.3.0, `xtrdb` |
| **H9 próg czasowy** | — | **poza zakresem, trwale** (niżej) |

Poziom wartości jest **warunkowy**: skrypt wykrywa, czym dysponuje host, i
wykonuje to, co da się wykonać. Brak zależności nie jest błędem — jest poziomem
niesprawdzonym, wypisanym z nazwy brakującego składnika
(`scripts/buildrdb.sh gate_requirements` je instaluje).

## Próg czasowy H9 — dlaczego go tu nie ma i nie będzie

Próg (redukcja bajtów substratu ≥40% wobec ablacji minimalnej i wobec planu
naturalnego, przy górnej granicy sparowanego bootstrap 95% CI ceny czasowej
≤1,05) jest wielkością **mierzoną, nie obliczaną**: 1440 komórek na przypiętej
maszynie brzegowej pod `PREEMPT_RT`, około 48 godzin zegara.

Kuszące jest wpięcie tu gotowej macierzy P8 i przepuszczenie jej przez zamrożone
[`h9/verdict.py`](h9/verdict.py). **Byłoby to gorsze niż nic:** macierz zmierzono
na innej rewizji silnika, więc zielone światło dotyczyłoby własności, której
bieżący silnik nigdy nie dotknął — czyli dokładnego przeciwieństwa weryfikacji
dryftu. Dryft progu czasowego wykrywa kampania pomiarowa, nie to polecenie.

## Brudne drzewo — poziom wartości i pin proweniencji

Weryfikacja dryftu robi się **w trakcie pracy nad silnikiem**, czyli na drzewie
brudnym. Na takim drzewie bramka `corpus_validity` nie ma jak przejść:
`validate_corpus.py --allow-dirty` stempluje dowód pinem `<sha>-dirty`, a
`run_gates.py` porównuje ten pin z czystym SHA i widzi rozjazd. Nie jest to
dryft, tylko brak proweniencji.

Skrypt schodzi z tego wąską i **sprawdzaną** ścieżką: musi zawieść dokładnie
`corpus_validity`, a każda rozbieżność zgłoszona w sekcji STOP-6 musi być tym
jednym pinem — to samo SHA po obu stronach, różne tylko sufiksem `-dirty`.
Cokolwiek innego (inna bramka, inne SHA, dodatkowa linia) jest **DRYFTEM**.
Poziom dostaje wtedy status `ZGODNY-bez-prow`, a przebieg wypisuje, że wartości
są sprawdzone, a proweniencja nie. Merytoryczna zawartość `corpus_validity` —
dokładny inwentarz 21 planów, 84/84 kompilacje, 4/4 mutanty, zamknięty manifest
— jest w tym samym przebiegu sprawdzona na poziomie mechanizmu.

Przebieg, który ma **nazywać rewizję**, robi się na czystym drzewie.

## Wyłączność przebiegu

Poziomy H9 piszą i czytają **dowód kompilacji** pod stałą ścieżką w drzewie
źródłowym — `h9/corpus_validation` — bo tam, i tylko tam, szuka go
`run_gates.py` (przez `validate_corpus.HERE`). Dwa przebiegi naraz nadpisałyby
sobie ten dowód: jeden kasowałby katalog w chwili, gdy drugi go czyta, a bramka
`corpus_validity` orzekałaby o cudzych plikach albo o pustce.

Dlatego przebieg zajmuje **wyłączną blokadę** `flock` na `.drift.lock`. Drugi
przebieg nie czeka w kolejce — kończy się kodem 2 i wypisuje, kto blokadę
trzyma (pid, czas startu, pełne wywołanie). Blokada zwalnia się z zamknięciem
deskryptora, więc przebieg ubity albo przerwany nie zostawia jej do ręcznego
sprzątania.

Blokada wyklucza drugi **przebieg**, ale nie wyklucza katalogu zostawionego
przez przebieg przerwany albo przez ręcznie uruchomione `validate_corpus.py`.
Dlatego przed odświeżeniem dowodu skrypt sprawdza jego **układ**: kasuje tylko
katalog, który ma `manifest.sha256` i `corpus-validation.tsv`, czyli własny.
Katalog o innej zawartości zostaje nietknięty, poziom mechanizmu kończy się jako
**NIESPRAWDZONY**, a decyzję o tym katalogu podejmuje człowiek.

## Dziennik

`DRIFT_JOURNAL.tsv` jest dopisywany, nigdy nadpisywany, i mówi, **która rewizja
silnika była fizycznie sprawdzona i z jakim wynikiem** — także wtedy, gdy wynikiem
był dryft. Bez niego nie da się odpowiedzieć na pytanie „od kiedy to jest zepsute",
bo każdy przebieg zna tylko siebie.

Powtórzenie przebiegu na tej samej rewizji nie jest nadużyciem: ziarno jest inne,
więc drugi przebieg poszerza pokrycie. Skrypt odnotowuje, że wpisów o tej rewizji
będzie kilka.

## Reguła decyzyjna H10 — skąd się wzięły progi

[`h10/decision_rule.py`](h10/decision_rule.py) przepisuje do kodu progi
predeklaracji K24 §6, w brzmieniu utrwalonym w [`h10/VERDICT.md`](h10/VERDICT.md):

* **H10a** — zgodność 100% w klasie jest jedynym wsparciem w tej klasie; jedna
  niezgodność falsyfikuje. Werdyktem jest atrybucja **izolowana**, nie
  propagowana. Osobno dla ogona startowego i osobno dla początku logicznego.
* **reżim zaniżający** — zawsze wynik negatywny, także gdyby odniesienie już go
  zawierało: to rekord wyemitowany, zanim jego zależności są określone.
* **H10b** — rozjazd reguły lokalnej A na ≥5% planów **oraz** 100% rozjazdów
  dodatnich o postaci `ceil((p+q-1)/p)`, **pod warunkiem** że predeklarowane
  kontrole negatywne `HC_SINGLE` i `HC_INT` są spełnione i mają niepustą
  populację. Na obecnej aparaturze `HC_INT` jest złamana, więc człon (b) jest
  **NIEOCENIALNY** — nie liczy się ani za H10, ani przeciw.

Metryki nie są przepisywane: `classify`, `classify_origin`, `regime`,
`member_b` i `controls` są importowane z [`h10/verdict.py`](h10/verdict.py).
Dwa przepisania tej samej definicji rozjeżdżają się po cichu.

Samotest (`--selftest`) obejmuje dziesięć przypadków o znanej odpowiedzi: trzy
wersje celowo zdryfowane (klasa zawyża ogon, klasa zaniża ogon, klasa myli
początek logiczny), trzy prowadzące do BRAKU WERDYKTU, oraz **kontrolę mocy
członu (b)** — gdyby żadne dane nie mogły uczynić go ocenialnym, jego progi
byłyby martwą gałęzią, a status `NIEOCENIALNY` tautologią.

## Zasada, na której to stoi

Ta sama co przy bramce: **nie poprawia się reguły, żeby wyszło na zielono.**
Wykryty dryft jest informacją o silniku i wchodzi do dziennika dokładnie tak samo
jak jego brak. Fałszywa czerwień jest równie zła jak fałszywa zieleń — dlatego
zejście z pinu proweniencji jest wąskie i sprawdzane, a nie założone.
