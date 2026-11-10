# Okna i serializacja

Uruchom z dowolnego katalogu, z `xretractor` i `xtrdb` w `PATH`:

```bash
bash /sciezka/do/retractordb/examples/mwd/test-mwnd.sh
bash /sciezka/do/retractordb/examples/mwd/test-mwnd2.sh
```

Skrypty kompilują program, wykonują 30 iteracji bez czekania na zegar i pokazują
pierwszych osiem rekordów każdego wyniku. Pracują na kopiach danych w osobnym
katalogu tymczasowym, usuwanym przy zakończeniu. Nie wymagają tmux ani aktywnego
terminala. Zmienne `XRETRACTOR` i `XTRDB` pozwalają wskazać konkretne binarki.

Pierwszy argument wybiera program, drugi limit iteracji albo samą kompilację:

```bash
bash examples/mwd/test-mwnd.sh query-mwnd3.rql 40
bash examples/mwd/test-mwnd2.sh query-mwnd2.rql --compile-only
```

Dawne `x` jako drugi argument oznacza to samo co `--compile-only` i po poprawnej
kompilacji zwraca kod 0. Nazwę programu można podać względem katalogu wywołania
lub, jeśli tam go nie ma, względem katalogu skryptu.

| Program | Co pokazuje |
|---|---|
| `query-mwnd.rql` | Okno `@(2,-2)` zachowuje kolejność napływu, a `@(1,1)` serializuje jego pola. Pierwsze okna to `(20,30)`, `(40,50)`, `(60,70)`. |
| `query-mwnd2.rql` | Cztery kombinacje kroku i znaku szerokości. Okna `@(2,3)` i `@(2,-3)` zaczynają się od `(3,2,1)` i `(1,2,3)`. |
| `query-mwnd3.rql` | Ten sam układ okna i serializacji z interwałem wejścia 1 s. |
| `query-mwnd4.rql` | Serializacja źródła mającego dwa pola w rekordzie. |

`@(k,w)` przesuwa okno o `k` elementów, a jego szerokość wynosi `abs(w)`.
Dodatnie `w` umieszcza najnowsze pole na początku, ujemne zachowuje kolejność
napływu. Okno kończy się na pozycji `n*k`; niepełne okna przed `origin` nie są
emitowane. Dlatego pierwsze okno `@(2,-2)` to `(20,30)`, a nie `(10,20)`.
Źródła bez `ONESHOT` zapętlają się po końcu pliku.

`datafile3.dat` zawiera dokładnie osiem 4-bajtowych liczb całkowitych 1..8,
bez separatorów i końca linii, w porządku bajtów little-endian używanym przez
platformy x86_64 i aarch64 tego projektu.
