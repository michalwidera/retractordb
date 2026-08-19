# Bramka badawcza — ochrona wyników przy rozwoju silnika

```bash
ninja test_gate          # z build/Debug lub build/Release
```

## Po co to jest

RetractorDB jest projektem żywym. Dwie hipotezy — **H9** i **H10** — zostały
zbadane i mają wyniki, na które powołuje się artykuł. Kod, który te wyniki
wytworzył, zmienia się dalej.

Ta bramka istnieje po to, żeby **rozwój źródeł był zapewniony**: zmiana ma
prawo poprawiać zachowanie silnika, a bramka wychwytuje **regresję względem
osiągniętych wyników**. Nie jest częścią eksperymentu, nie jest dowodem
i nie potwierdza hipotez — kampania jest dowodem dlatego, że była
predeklarowana i zamrożona **przed** pomiarem, a test regresji uruchamiany po
fakcie tej roli pełnić nie może.

Bramka jest **kierunkowa**:

| Kierunek | Reakcja |
|---|---|
| wynik gorszy niż odniesienie | **BŁĄD** — praca się zatrzymuje |
| wynik lepszy niż odniesienie | **POPRAWA** — bramka przechodzi, zmiana zostaje odnotowana |
| wynik równy | przechodzi |

## Co jest weryfikowane

| Katalog | Hipoteza | Co chroni |
|---|---|---|
| [`h10/`](h10/README.md) | **H10** — początek logiczny i ogon startowy dają się policzyć statycznie z planu i zgadzają się z modelem zdarzeniowym | podział dziewięciu klas operatorów na reżimy: **9 dokładnych, 0 zawyżających, 0 zaniżających** (do 2026-08-18: 6/3/0); początek logiczny dokładny w 9/9 |
| [`h9/`](h9/README.md) | **H9** — kompilator rozpoznaje automatycznie wspólny materializowany podplan i liczy go raz zamiast wielokrotnie | mechanizm rozpoznawania równoważnych obliczeń: korpus, tablica mechanizmu, 84 kompilacje na czterech profilach ablacji |

Szczegóły — pełne brzmienie hipotez, przebieg i reguły oblania — w README
każdego katalogu.

## Jak jest wpięta

Celowo **poza `ninja`** i **poza `ninja test`**: codzienna praca przy źródłach
nie ma się o nią zatrzymywać. Uruchamia się jawnie, z okresowego joba CI albo
ręcznie przed wysłaniem artykułu do recenzji.

```bash
scripts/buildrdb.sh gate_requirements              # zaleznosci: JDK 17 + Flink 2.3.0
ninja test_gate                                    # calosc
./run_gate.sh --only h10                           # sam H10
./run_gate.sh --only h9 --profiles <katalog>       # H9 z profilami ablacji
./run_gate.sh --count 1000                         # skrocona kampania H10
./run_gate.sh --strict ...                         # poziom pominiety = oblany
```

`--profiles` przyjmuje sciezke uzywana po `cd` do katalogu poziomu, wiec podaje
sie ja bezwzglednie; sciezka wzgledna cicho pomija poziom 84/84.

**W CI** bramka ma wlasny job `research-gate` (warstwa 3 workflow
`manual-nightly-full`, trzy razy w miesiacu, Release). Job buduje najpierw
cztery profile ablacji, bo bez nich poziom 84/84 jest pomijany, i konfiguruje
drzewo z `-DRESEARCH_GATE_STRICT=ON`. W tym trybie **pominiety poziom oblewa
przebieg**: pominiecie znaczy "nie uruchomiono", a zielona bramka o niepelnym
zakresie jest gorsza niz jej brak. Praca lokalna zostaje przy domyslnym OFF,
gdzie pominiecie jest tylko odnotowane.

Ten sam job wchodzi takze do workflow `research-gate-only`, uruchamianego z reki
przez parametr pipeline'u `run_manual_research_gate` — sama bramka, bez warstw L1
i L2, na potrzeby sprawdzenia przed zgloszeniem artykulu. Jest to dodatek, nie
zamiennik: przebieg nocny daje bramce ccache zapisany dla tej samej rewizji,
uruchomienie samodzielne startuje z cache'a starszego.

Poziomy mają różne wymagania i `gate_requirements` instaluje te najwyższego:

| Poziom | Wymaga |
|---|---|
| H10 oraz podstawowe kontrole H9 | `python3` i zbudowany `xretractor` |
| H9, 84 kompilacje | cztery profile ablacji |
| H9, oracle wartości wobec Flinka | JDK 17 i Flink 2.3.0 |

* pełny przebieg — około 70 s (dwa ziarna × 10 010 planów H10 plus cztery
  kontrole H9);
* liczbę planów zmienia `--count` albo `-DRESEARCH_GATE_COUNT=` przy `cmake`;
* skrypt działa na katalogu **źródłowym**, a pisze wyłącznie do katalogu
  buildu — edycja aparatury nie wymaga ponownego `cmake .`;
* przebieg nie zostawia śladów w drzewie: `__pycache__`, `work/` i
  `.gate-work/` są w `.gitignore`.

## Zasada, na której to stoi

**Nie poprawia się bramki, żeby zaświeciła na zielono.** Czerwone światło jest
informacją o zmianie zachowania silnika, a nie usterką testu. Jeżeli zmiana
była zamierzona, plik odniesienia aktualizuje się **w tym samym commicie**, co
zmiana silnika, wraz z odnotowaniem, co się zmieniło i dlaczego. Podmiana
wartości bez tego zapisu kasuje jedyną ochronę, jaką ten katalog daje.

Bramka wypisuje na końcu każdego przebiegu listę tego, czego **nie**
sprawdziła — w szczególności progów czasowych H9, które wymagają
kilkudziesięciogodzinnego pomiaru na przypiętej maszynie brzegowej. Zielone
światło znaczy „zachowanie nie cofnęło się", nie „hipoteza potwierdzona".
