#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
./link-lake.sh

# --wfail: `sorry` jest w Lean tylko ostrzezeniem, wiec bez tej flagi niedokonczony dowod by przeszedl.
if ! lake build Profs profs_manual --wfail; then
  printf '\nDOWODY: BLAD - kod Lean lub dokumenty Verso maja bledy albo ostrzezenia (w tym sorry); szczegoly powyzej.\n' >&2
  exit 1
fi
trap 'printf "\nRENDEROWANIE: BLAD - dowody sa poprawne, ale nie udalo sie wygenerowac wynikow; szczegoly powyzej.\n" >&2' ERR

lake exe profs_manual --with-tex

# Wyjscie latexmk tylko do logu; na ekran trafia wylacznie przy bledzie.
if ! (cd build/verso/tex && latexmk -lualatex -interaction=nonstopmode -halt-on-error -silent main.tex) \
  >build/verso/latexmk.log 2>&1; then
  cat build/verso/latexmk.log >&2
  false
fi

mkdir -p build/verso/text
python3 render_text.py build/verso/html-single/index.html \
  build/verso/text/all-proofs.txt

for chapter in interleave-covering deinterleave exact-invertibility event-order sum-commutativity shift-matching; do
  python3 render_text.py "build/verso/html-multi/$chapter/index.html" \
    "build/verso/text/$chapter.txt"
done

printf 'HTML: %s\n' "$(pwd)/build/verso/html-single/index.html"
printf 'Text: %s\n' "$(pwd)/build/verso/text/all-proofs.txt"
printf 'PDF:  %s\n' "$(pwd)/build/verso/tex/main.pdf"
printf '\nDOWODY: OK - kod Lean i dokumenty Verso sprawdzone bez bledow i ostrzezen.\n'
