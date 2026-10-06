---
name: watermark-check
description: Recepty watermarks-remover - sprawdzenie i usunięcie znaków wodnych AI (niewidoczne Unicode, homoglify spacji, konfuzable) w plikach tekstowych repozytorium. Wywołaj przed każdym commitem i pushem, oraz natychmiast po edycji pliku źródłowego. Rozdziela skan ścisły i naprawę punktową kodu od czyszczenia prozy; zawiera sekwencję dla plików zastagowanych, wariant całego drzewa i reguły czego nie wolno czyścić.
---

# Watermark check - recepty

Reguła obowiązująca (kiedy i dlaczego) jest w `CLAUDE.md`, sekcja „AI watermark hygiene (text)". Używaj lokalnych skryptów, bez uruchamiania Docker/HTTP i bez przepisywania tekstu przez statystyczną warstwę B.

## Kod - natychmiast po edycji

Każdy plik źródłowy sprawdź zaraz po edycji, przed `ninja cformat` i budowaniem, a także przed commitem i pushem. Dotyczy to C/C++, gramatyk, RQL, deskryptorów, skryptów i konfiguracji wymienionych we wzorcu `SRC` w `scripts/hygiene-check.sh`, łącznie z szablonami `.in`. Nie wklejaj do kodu surowego tekstu z modelu, przeglądarki ani czatu: przepisz go jako ASCII lub oczyść przed zapisaniem.

```bash
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
f='path/to/source.cpp'
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue "$f"
```

Trafienie zatrzymuje pracę: zgłoś człowiekowi plik, linię i codepoint. Raport narzędzia podaje offsety znaków od zera; linię ustal jako 1 + liczbę znaków nowej linii przed danym offsetem. Każdy `U+00A0` lub znak niewidoczny w kodzie jest defektem, nawet jeśli raport nazywa go `informational`. Błąd narzędzia także blokuje uznanie pliku za czysty.

Naprawiaj tylko zgłoszony defekt. Nie uruchamiaj zbiorczego czyszczenia ani `--in-place` dla kodu. Kandydat do naprawy powstaje w osobnym pliku:

```bash
python3 "$WM/clean_text.py" "$f" --aggressive-homoglyphs --strip-emoji-glue -o "$f.fixed"
git diff --no-index -- "$f" "$f.fixed"
```

`git diff --no-index` zwraca 1, gdy pliki się różnią - to wynik do oceny, nie zgoda na zastąpienie. Sprawdź, że zmienił się tylko zgłoszony codepoint. Jeśli narzędzie zmieniło coś jeszcze, nie kopiuj kandydata; wykonaj naprawę punktową i ponownie porównaj. Dopiero po tej kontroli i czystym skanie kandydata:

```bash
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue "$f.fixed"
# Po wyniku 0 powyzej i sprawdzeniu diffu:
cp -- "$f.fixed" "$f" && rm -- "$f.fixed"
git diff -- "$f"
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue "$f"
```

## Proza - kontrolowane czyszczenie

Dla dokumentacyjnego Markdown, tekstu, LaTeX i bibliografii używaj skanu domyślnego. Najpierw obejrzyj raport i kontekst trafień. Wyjątek ikony z `README.md` opisany poniżej pozostaw bez zmian.

`clean_text.py` zamienia każdą `U+00A0` na zwykłą spację. Jeśli raport wskazuje celową spację typograficzną, dodaj `--no-normalize-spaces`: usunie znaki niewidoczne, a spację zostawi. Zostawiona `U+00A0` nadal jest trafieniem skanu i blokuje commit, więc zgłoś ją człowiekowi z plikiem i linią.

```bash
f='path/to/document.md'
python3 "$WM/inspect_text.py" "$f"
# Dopiero po ocenie trafien:
python3 "$WM/clean_text.py" "$f" --in-place --stats
git diff --no-index -- "$f.bak" "$f"
git diff -- "$f"
python3 "$WM/inspect_text.py" "$f"
```

Sprawdź, że diff zawiera wyłącznie zamierzone usunięcia lub zamiany codepointów, bez zmiany widocznej treści. Jeśli czyszczenie zmieniło coś innego, przywróć plik z `.bak` i napraw tylko trafienie. Kopię `.bak` usuń dopiero po kontroli diffu i ponownym skanie; nigdy jej nie commituj.

## Przed commitem i pushem

Kontrolę wykonuje `scripts/hygiene-check.sh`: pliki z wzorca `SRC` skanuje w trybie ścisłym, pozostałe pliki tekstowe w domyślnym, a wypisuje tylko trafienia. Nie zastępuje natychmiastowego skanu po edycji kodu. Skanowane pliki robocze muszą odpowiadać wersjom w indeksie: po naprawie sprawdź diff, ponownie zastaguj dany plik w ramach autoryzowanego stagingu i powtórz kontrolę przed commitem.

```bash
scripts/hygiene-check.sh staged path/to/commit-message.txt   # przed commitem
scripts/hygiene-check.sh tree                                 # przed pushem: cale sledzone drzewo
git log -1 --pretty=%B | scripts/hygiene-check.sh tree -      # przed pushem, z wiadomoscia commita
```

Kod wyjścia 0 oznacza czysto, 1 trafienia, 2 błąd narzędzia lub argumentu. Trafienie lub błąd odczytu pliku trafia do linii `watermarks: HITS`; pliki oznaczone `(strict)` to kod - zatrzymaj się i zgłoś je zgodnie z sekcją kodu. Trafienia w prozie rozpatrz według sekcji prozy. Pozostałe linie raportu (`formatting`, `leftovers`, `dashes`) blokują commit tak samo. Nie oddawaj diffu do review ani nie wykonuj commita/pusha, dopóki raport nie jest czysty.

Te kontrole nie udzielają zgody na commit, push ani CI; obowiązuje osobna autoryzacja z `CLAUDE.md`.

## Tryb ścisły dla kodu - dlaczego `--aggressive`

Default mode misses Latin/Cyrillic confusables: `int value = 1;` whose `a` is a Cyrillic `U+0430` instead of ASCII `a` passes it and is caught only by `--aggressive`. (Write such an example by naming the codepoint - never paste the actual character into a rule file, a comment or a test.) `--strip-emoji-glue` additionally rejects the load-bearing invisibles that are legitimate in prose but never in code. Verified against the whole `src/` and `scripts/` tree: strict mode yields zero hits, and Polish diacritics in comments are not affected.

## Reguły - czego nie wolno czyścić

- **Never run `clean_text.py` on binary fixtures** (`test/**/*.dat`, `.meta`, `.shadow`, ECG records, `examples/**` data files). It rewrites bytes and corrupts them, and integration tests compare output byte-exactly. The extension filter (`TEXT` in `scripts/hygiene-check.sh`) exists for that reason - do not widen it with `--force-text`.
- `--in-place` jest dopuszczalne tylko dla prozy po ocenie trafień. Tworzy `.bak`; usuń ją dopiero po kontroli diffu i ponownym skanie. Plików `.bak` i `.fixed` nie commituj.
- `U+00A0` (no-break space) is reported as *informational*. W każdym pliku kodu jest defektem, bez wyjątków. W prozie najpierw potwierdź, że nie jest celową spacją typograficzną.
- If cleaning would change test fixtures or generated ANTLR files, stop and hand the case to the human instead of editing them.
- The `U+FE0F VARIATION SELECTOR-16` in the warning icon before `**This is work in progress:**` in `README.md` is the narrow Markdown exception recorded in `CLAUDE.md`. Keep that icon unchanged. On a strict scan, inspect the codepoint and its position; do not exempt any other hit.
