# Dowody RetractorDB w Lean 4

Ten katalog jest samodzielnym projektem Lean 4. Zawiera formalizacje
twierdzeń z [publikacji DEBS](../../paper-arXiv/debs/paper-debs2027.tex),
[długiej publikacji](../../paper-arXiv/arxiv/main.tex) (obie w repozytorium
`paper-arXiv`, gdzie jest też historia tego katalogu sprzed przeniesienia) i
[polskiej dokumentacji](../../dokumentacja-rdb/podstawy-matematyczne/formalne-podstawy-i-dowody.md).
Każdy dowód ma osobny plik Lean w `Profs/` i osobny dokument VersoManual
w `ProfsManual/`. `BeattyModel.lean` zawiera wspólne definicje, a
`BeattyPartition.lean` podział pozycji Beatty'ego, z którego korzysta
Twierdzenie 2 (pokazany w jego dokumencie). `CausalShift.lean` rozszerza R1
o przesunięcie przyczynowe, początki logiczne i dostępność rekordów, a
`InterleaveTailExact.lean` dowodzi okresowości wymagań fazowych i dokładności
ogona przeplotu jako granicy dostępności.

| Dowód | Lean | Dokument Verso |
| --- | --- | --- |
| Twierdzenie 1: przeplot jest pokryciem sekwencyjnym | `Profs/InterleaveCovering.lean` | `ProfsManual/InterleaveCovering.lean` |
| Twierdzenie 2: rozplot spełnia warunki Fraenkela | `Profs/Deinterleave.lean` | `ProfsManual/Deinterleave.lean` |
| Wniosek: dokładna odwracalność na liczbach wymiernych | `Profs/ExactInvertibility.lean` | `ProfsManual/ExactInvertibility.lean` |
| Stwierdzenie: zaburzenie kolejności zdarzeń | `Profs/EventOrder.lean` | `ProfsManual/EventOrder.lean` |
| Stwierdzenie: przemienność sumy strumieni | `Profs/SumCommutativity.lean` | `ProfsManual/SumCommutativity.lean` |
| Twierdzenie R1: przesunięcie dopasowane do tempa | `Profs/ShiftMatching.lean`, `Profs/CausalShift.lean`, `Profs/InterleaveTailExact.lean` | `ProfsManual/ShiftMatching.lean` |

Twierdzenie Fraenkela (1969) publikacja cytuje bez dowodu, więc nie ma
osobnego dokumentu.

Pierwsze uruchomienie pobiera Lean 4.33.0, Mathlib i Verso 4.33.0:

```bash
cd math_proofs
./install-lean.sh   # elan, Lean z lean-toolchain, pandoc, cache Mathlib
lake update
./render.sh
```

`./install-lean.sh` sprawdza przy każdym uruchomieniu, czy jest nowszy elan
oraz nowsza stabilna wersja (bez `-rc`) wydana wspólnie przez Lean, Mathlib
i Verso. W terminalu pyta, czy ją zainstalować; bez terminala tylko o niej
informuje. `--upgrade` instaluje bez pytania, `--no-upgrade-check` pomija
sprawdzenie. Podniesienie wersji zmienia tagi w `lakefile.lean`, a
`lake update` przepisuje `lean-toolchain`. Jeśli `lake build --wfail`
zgłosi błąd albo ostrzeżenie (na przykład przestarzały lemat Mathlib),
skrypt przywraca poprzednie wersje.

Katalog roboczy Lake (`.lake`: Mathlib, Verso i wyniki budowania, kilka GB)
nie leży w repozytorium. `./link-lake.sh` przenosi go do
`~/.cache/retractordb-lean/profs-<skrót ścieżki projektu>/.lake` i zostawia
tu tylko dowiązanie `.lake`; inny katalog bazowy ustawia `RDB_LEAN_CACHE`.
Skrypt wołają `install-lean.sh`, `render.sh` i `gen-oracle.sh`, więc zwykle
nie trzeba go uruchamiać ręcznie. Dowiązanie jest ignorowane przez git
(wzorzec `.lake` bez końcowego ukośnika, bo taki pasuje tylko do katalogu).

Wynik HTML jest w `build/verso/html-single/index.html` oraz jako sześć
osobnych stron w `build/verso/html-multi/`. Wersja tekstowa jest w
`build/verso/text/all-proofs.txt` i sześciu plikach tekstowych, po jednym
na dowód. PDF jest w `build/verso/tex/main.pdf` (LaTeX z Verso składany
przez `lualatex`). Skrypt wymaga `pandoc` do konwersji HTML na zwykły tekst
oraz `latexmk` i pakietów TeX Live do PDF; `install-lean.sh` je doinstalowuje.
Samo `lake build Profs` sprawdza kod dowodów, a `lake build profs_manual`
sprawdza także dokumenty i zgodność wstawionych fragmentów kodu z plikami
źródłowymi.

## Wyrocznia dla testów silnika

`OracleMain.lean` (plik wykonywalny `profs_oracle`) liczy definicjami z
`Profs/` tablice przeplotu, rozplotu, różnicy i dokładnego ogona przeplotu.
`./gen-oracle.sh` zapisuje je razem ze skrótami SHA-256 plików `Profs/*.lean`,
`OracleMain.lean` i przypięć wersji (`lakefile.lean`, `lean-toolchain`,
`lake-manifest.json`) do `test/UnitTest/proofOracle.hpp`. Tam `ut_proofOracle`
porównuje z nimi funkcje z `SOperations.hpp` i sprawdza wypowiedzi twierdzeń
jako własności, a `proof_drift` pilnuje, żeby każde twierdzenie miało wiersz
w `test/proof_manifest.tsv` i żeby tablice nie były starsze od dowodów. Po
każdej zmianie tych plików, także po `./install-lean.sh --upgrade`, trzeba
ponownie uruchomić `./gen-oracle.sh`, a nowe
twierdzenie dopisać do manifestu.

Formalizacja wykorzystuje uogólnione twierdzenie Rayleigha z Mathlib do
wykazania podziału dodatnich pozycji dla wymiernych parametrów.
Wszystkie twierdzenia zależą wyłącznie od standardowych aksjomatów
(`propext`, `Classical.choice`, `Quot.sound`); obliczenia sprawdza jądro
(`decide`, `decide +kernel`), bez `native_decide`. Nie
formalizuje pełnego twierdzenia Fraenkela. Dowody dotyczą indeksów,
wartości, odstępów i fazowego wzoru ogona. Dla R1 obejmują również
przesunięcie odczytujące `n-m`, brak rekordów przed początkiem logicznym,
równość początków i odczytów oraz dostępność nie później po faktoryzacji.
Maksimum fazowe po jednym okresie jest udowodnione jako najmniejszy ogon
spełniający zdarzeniowy warunek dostępności dla wszystkich rekordów od
dowolnego progu. Awaryjne oszacowanie ogona powyżej progu przeglądu pozostaje
poza formalizacją. Zgodność całej implementacji
C++ z modelem, w tym deskryptorów, map `NULL`, luk i materializacji, pozostaje
oddzielnym obowiązkiem weryfikacyjnym.
