---
name: watermark-check
description: Recepty watermarks-remover — sprawdzenie i usunięcie znaków wodnych AI (niewidoczne Unicode, homoglify spacji, konfuzable) w plikach tekstowych repozytorium. Wywołaj przed każdym commitem i pushem, oraz po edycji pliku źródłowego. Zawiera sekwencję dla plików zastagowanych, wariant dla całego drzewa przy pushu, wyjaśnienie trybu ścisłego dla kodu i reguły czego nie wolno czyścić.
---

# Watermark check — recepty

Reguła obowiązująca (kiedy i dlaczego) jest w `CLAUDE.md`, sekcja „AI watermark hygiene (text)".
Ten plik zawiera same polecenia.

## Przed commitem i pushem

```bash
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
TEXT='\.(md|txt|tex|bib|rql|desc|cpp|hpp|h|c|g4|sh|py|ya?ml|toml|json|cmake|in)$|CMakeLists\.txt$'

# 1. Check the staged text files (empty output = clean)
git diff --cached --name-only --diff-filter=ACM | grep -E "$TEXT" \
  | while read -r f; do python3 "$WM/inspect_text.py" --json "$f" >/dev/null 2>&1 || echo "WATERMARK: $f"; done

# 2. Report for a flagged file (which codepoints, where)
python3 "$WM/inspect_text.py" <file>

# 3. Clean it, then drop the backup the tool leaves behind
python3 "$WM/clean_text.py" <file> --in-place --stats && rm -f <file>.bak

# 4. Re-check, then re-stage
python3 "$WM/inspect_text.py" --json <file> >/dev/null && git add <file>
```

Before a push, run the same check over the whole tracked tree — substitute `git ls-files` for
`git diff --cached --name-only --diff-filter=ACM` in step 1. Optionally check the commit message too:
`git log -1 --pretty=%B | python3 "$WM/inspect_text.py" -`.

## Tryb ścisły dla kodu — dlaczego `--aggressive`

Default mode misses Latin/Cyrillic confusables: `int value = 1;` whose `a` is a Cyrillic `U+0430` instead of
ASCII `a` passes it and is caught only by `--aggressive`. (Write such an example by naming the codepoint —
never paste the actual character into a rule file, a comment or a test.) `--strip-emoji-glue` additionally rejects the load-bearing invisibles that
are legitimate in prose but never in code. Verified against the whole `src/` and `scripts/` tree: strict
mode yields zero hits, and Polish diacritics in comments are not affected.

## Reguły — czego nie wolno czyścić

- **Never run `clean_text.py` on binary fixtures** (`test/**/*.dat`, `.meta`, `.shadow`, ECG records,
  `examples/**` data files). It rewrites bytes and corrupts them, and integration tests compare output
  byte-exactly. The extension filter above exists for that reason — do not widen it with `--force-text`.
- `--in-place` writes a `.bak` next to the file. Delete it; never commit it.
- `U+00A0` (no-break space) is reported as *informational*. In `.rql`, `.g4` and C++ sources it is always a
  defect — normalize it. Elsewhere confirm it is not a deliberate typographic space before replacing.
- If cleaning would change test fixtures or generated ANTLR files, stop and hand the case to the human instead
  of editing them.
