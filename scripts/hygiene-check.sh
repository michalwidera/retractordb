#!/bin/bash
#
# Kontrola przed oddaniem diffu, commitem lub pushem (CLAUDE.md: "AI watermark
# hygiene", "Code Style"). Zastepuje agenta hygiene-check: wszystkie sprawdzenia
# sa deterministyczne, wiec nie potrzebuja modelu.
#
# Tylko raportuje - niczego nie czysci, nie formatuje i nie stage'uje.
#
# Uzycie: scripts/hygiene-check.sh [working|staged|tree] [plik-wiadomosci-commita|-]
#   working (domyslnie) - przed handoffem: git diff HEAD + pliki niesledzone
#   staged              - przed commitem: pliki zastage'owane
#   tree                - przed pushem: cale drzewo sledzone
# Kod wyjscia: 0 = czysto, 1 = trafienia, 2 = blad (brak narzedzia, zly argument).

set -o nounset
set -o pipefail

scope=${1:-working}
msg=${2:-}
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 2
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
[ -f "$WM/inspect_text.py" ] || { echo "ERROR: brak $WM/inspect_text.py"; exit 2; }

case "$scope" in
working) files=$( (git diff HEAD --name-only --diff-filter=ACM; git ls-files --others --exclude-standard) | sort -u) ;;
staged) files=$(git diff --cached --name-only --diff-filter=ACM) ;;
tree) files=$(git ls-files) ;;
*) echo "ERROR: nieznany zakres '$scope' (working|staged|tree)"; exit 2 ;;
esac

TEXT='\.(md|txt|tex|bib|rql|desc|cpp|hpp|h|c|g4|sh|py|ya?ml|toml|json|cmake|in)$|CMakeLists\.txt$'
SRC='\.(cpp|hpp|h|c|g4|rql|desc|sh|py|cmake|toml|ya?ml|json)$|CMakeLists\.txt$'
status=0

report() { # nazwa, trafienia (jedno na linie)
  if [ -z "$2" ]; then
    echo "$1: CLEAN"
  else
    echo "$1: HITS ($(printf '%s\n' "$2" | grep -vc '^ '))"
    printf '%s\n' "$2" | sed 's/^/  /'
    status=1
  fi
}

echo "scope: $scope ($(printf '%s' "$files" | grep -c .) plikow)"
git status --short

# Znaki wodne: tryb domyslny dla tekstu, tryb scisly dla kodu.
hits=""
while read -r f; do
  [ -f "$f" ] || continue
  if printf '%s' "$f" | grep -qE "$SRC"; then
    out=$(python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue "$f") || hits+="$f (strict):"$'\n'"$(printf '%s\n' "$out" | grep '^  \[')"$'\n'
  else
    python3 "$WM/inspect_text.py" --json "$f" >/dev/null 2>&1 || hits+="$f: $(python3 "$WM/inspect_text.py" "$f" | grep '^  \[' | tr -s ' ' | paste -sd ';')"$'\n'
  fi
done < <(printf '%s\n' "$files" | grep -E "$TEXT")
if [ -n "$msg" ]; then
  out=$( (if [ "$msg" = - ]; then cat; else cat "$msg"; fi) | python3 "$WM/inspect_text.py" -) || hits+="commit message:"$'\n'"$(printf '%s\n' "$out" | grep '^  \[')"$'\n'
fi
report watermarks "${hits%$'\n'}"

# Formatowanie: te same narzedzia co cel cformat w src/CMakeLists.txt, bez zmian w plikach.
hits=""
cxx=$(printf '%s\n' "$files" | grep -E '^(src|test)/.*\.(cpp|hpp|h|cc)$' | grep -v '\.antlr/')
cml=$(printf '%s\n' "$files" | grep -E '(^|/)CMakeLists\.txt$')
for tool in ${cxx:+clang-format} ${cml:+cmake-format}; do
  command -v "$tool" >/dev/null || { echo "ERROR: brak $tool"; exit 2; }
done
while read -r f; do
  [ -f "$f" ] && { clang-format --dry-run --Werror "$f" >/dev/null 2>&1 || hits+="$f"$'\n'; }
done <<<"$cxx"
while read -r f; do
  [ -f "$f" ] && { cmake-format --check --line-width=80 "$f" >/dev/null 2>&1 || hits+="$f"$'\n'; }
done <<<"$cml"
report formatting "${hits%$'\n'}"

report leftovers "$(git ls-files --others --exclude-standard | grep -E '\.(bak|fixed|orig|rej)$')"

# Typograficzne myslniki w dodanych liniach; czy to cytat, rozstrzyga wywolujacy.
added() { awk '/^\+\+\+ b\//{f=substr($0,7);next} /^\+/{print f": "substr($0,2)}'; }
case "$scope" in
working) dashes=$(git diff HEAD | added; git ls-files --others --exclude-standard -z | xargs -0 -r grep -HnI '') ;;
staged) dashes=$(git diff --cached | added) ;;
tree) dashes=$(git diff '@{upstream}...HEAD' 2>/dev/null | added) ;;
esac
report dashes "$(printf '%s\n' "$dashes" | LC_ALL=C.UTF-8 grep -P '[\x{2013}\x{2014}]')"

exit $status
