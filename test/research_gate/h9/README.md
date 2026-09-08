# Bramka H9 — mechanizm współdzielenia materializowanego podplanu

## Hipoteza, której wyniki ta bramka chroni

**H9.** Kompilator rozpoznaje **automatycznie**, że kilka strumieni
publicznych liczy równoważne obliczenie, i materializuje wspólny podplan raz
zamiast wielokrotnie — dochodząc samodzielnie do planu, który w systemie bez
tego mechanizmu trzeba wypisać ręcznie.

Mechanizm składa się z dwóch przejść kompilatora, które bramka rozróżnia:

| Przejście | Flaga budowania | Co robi |
|---|---|---|
| **R1** | `RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES` | wynosi zgodne co do tempa przesunięcia przed przeplot |
| **R2** | `RDB_OPT_COMMUTATIVE_ADD` | kanonizuje kolejność dzieci węzła sumy strumieni, żeby równoważne `SELECT` dały ten sam odcisk |

Wynik kampanii, który ta bramka chroni: **H9 wsparta w klasie `Q=8`, w trzech
rodzinach na trzy**, gdzie `Q` jest liczbą równoważnych postaci monitora.
Rodziny izolują przejścia i ich złożenie: `F9-R2` samo R2, `F9-R1` samo R1,
`F9-X` złożenie R1 → R2.

**Upoważnienie jest ograniczone do zbadanej klasy.** Nie jest to twierdzenie
o ogólnej przewadze wydajnościowej ani o tym, że inne systemy nie potrafią
współdzielić.

## Na czym polega bramka

To **nie jest** powtórzenie kampanii i nie potwierdza H9 na nowym silniku.
To zabezpieczenie rozwoju: kod ma się dalej zmieniać, a bramka pilnuje, żeby
zmiana nie zepsuła mechanizmu ani nie zmieniła wyników publicznych.

| Krok | Co orzeka | Wymaga profili ablacji |
|---|---|---|
| `gen_corpus.py --check` | 37 plików korpusu nadal zgodnych z generatorem — dane i plany nie rozjechały się z kodem, który je wytwarza | nie |
| `validate_corpus.py --selftest` | bramka korpusu odrzuca pominięty, zdublowany, nadmiarowy i historycznie nielegalny plan | nie |
| `verdict.py --selftest` | procedura decyzyjna działa na przypadkach granicznych, w tym na metryce zdegenerowanej | nie |
| `mechanism_table.py --gate` | tablica mechanizmu o znanej odpowiedzi: licznik odróżnia prawdziwe współdzielenie od jego pozoru | nie |
| `validate_corpus.py` | **84 kompilacje** (21 planów × 4 profile) i **4 odrzucone** mutanty historyczne | tak |

Ostatni poziom wymaga czterech zbudowanych profili — `DEFAULT`,
`NO_R2_CANON`, `NO_R1_FACTOR`, `NO_R1_NO_R2`. Bez nich nie da się odróżnić
skutku R1 od skutku R2, więc bez `--profiles` bramka raportuje ten poziom jako
**POMINIĘTY**, nigdy jako zdany.

```bash
./build_profiles.sh                          # cztery buildy
../run_gate.sh --only h9 --profiles <katalog buildów>
```

**Profile muszą być świeże.** `xretractor --build-info` podaje wyłącznie flagi
optymalizatora, nie rewizję źródła, więc świeżość rozstrzyga **odcisk treści
`src/`**. `build_profiles.sh` zapisuje go przy budowie profilu w
`build/K26v3-<slug>/.gate-src-fingerprint`, a bramka porównuje z odciskiem
bieżącego drzewa. Obie strony liczą go tym samym kodem — `run_gate.sh
--print-src-fingerprint` — więc definicja odcisku jest jedna. Gdy odciski się
różnią, bramka automatycznie uruchamia `build_profiles.sh`, ponownie weryfikuje
odciski i wykonuje poziom 84/84. Gdy profilu brak, brak przy nim odcisku albo
odcisku nie da się policzyć, bramka **pomija poziom 84/84** zamiast orzekać.
Profil zbudowany z innej treści dałby zieleń, która nie mówi nic o badanej
rewizji.

Odcisk liczy się z **drzewa roboczego**, nie z `HEAD`, więc niezacommitowana
zmiana w `src/` unieważnia profile dokładnie tak samo jak commit. Miarą **nie
jest** czas modyfikacji: `git checkout` i `pull --rebase` przepisują mtime także
tym plikom, których treść się nie zmieniła, i dawna reguła mtime pomijała wtedy
poziom 84/84 bez powodu (2026-08-18 — profile zbudowane o 17:00 uznane za
nieświeże po rebase o 18:37, przy pustym `git diff -- src/`).

Zależności najwyższego poziomu instaluje `scripts/buildrdb.sh
gate_requirements` — przypięty JDK 17 i Flink 2.3.0, z weryfikacją sumy
SHA-512 archiwum i próbną kompilacją klas Flinka bramki.

**Brudne drzewo nie blokuje tego poziomu.** Aparatura kampanii odmawiała
pracy na drzewie roboczym, bo dowód kompilacji musi nazywać rewizję. Bramka
regresyjna pyta o co innego — czy 21 planów nadal kompiluje się w czterech
profilach — więc uruchamia się także w trakcie pracy, przez `--allow-dirty`.
Powstały dowód dostaje wtedy SHA z sufiksem **`-dirty`** i **nie jest dowodem
proweniencji**; to rozróżnienie jest zapisane w samym pliku dowodu, nie tylko
w tym README.

## Czego ta bramka nie sprawdza

1. **Progu czasowego.** Kampania wymagała redukcji bajtów substratu na rekord
   publiczny o co najmniej 40% wobec ablacji minimalnej i wobec naturalnego
   planu odniesienia, przy górnej granicy sparowanego bootstrap 95% CI ceny
   czasowej nie większej niż 1,05. Wymaga to 1440 komórek na przypiętej
   maszynie brzegowej pod `PREEMPT_RT`, około 48 godzin zegara. Tego nie da
   się uczciwie wcisnąć do bramki regresyjnej i nie należy próbować.
2. **Bramki `oracle_values`** — porównania wartości wobec niezależnego portu
   w Apache Flink. Wymaga Flink 2.3.0 i JDK 17.

Zielone światło znaczy **„mechanizm nadal działa tak samo"**, nie „hipoteza
potwierdzona na tym silniku".

## Gdy bramka oblewa

Czerwone światło na `mechanism_table --gate` albo na 84/84 znaczy, że zmiana
kodu dotknęła rozpoznawania równoważnych obliczeń. To nie musi być błąd —
może być zamierzona zmiana normalizacji planu. Ustal, która zmiana to
spowodowała, i rozstrzygnij świadomie. **Nie podmieniaj oczekiwanych
wartości, żeby zaświeciło na zielono**: to jedyne, co chroni wynik kampanii
przed cichym unieważnieniem.

Dowodu kompilacji z kampanii **nie ma w tym katalogu i nie powinno być**:
`corpus_validation/` to 183 pliki `.plan` i `.stderr` wytworzone przez
konkretną rewizję silnika, czyli wyjście uruchomienia, a nie źródło. Bramka
generuje własny dowód w katalogu buildu (`--out`). Ścieżka jest w
`.gitignore`, więc ręczny przebieg `validate_corpus.py` bez `--out` — który
domyślnie pisze właśnie tutaj — nie wprowadzi go do repozytorium.
