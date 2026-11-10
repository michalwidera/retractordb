#!/usr/bin/env bash
# Pokaz wsadowy czterech przykladow query-mwnd*.rql, bez zaleznosci od tmux.
# Uzycie: test-mwnd.sh [plik.rql] [liczba-iteracji | x | --compile-only]
# Domyslnie: query-mwnd.rql, 30 iteracji; x zachowuje dawny tryb kompilacji.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
query=${1:-"$script_dir/query-mwnd.rql"}
if [[ ! -f "$query" && -f "$script_dir/$query" ]]; then
  query="$script_dir/$query"
fi
query_dir=$(cd -- "$(dirname -- "$query")" && pwd)
query_name=$(basename -- "$query")
mode=${2:-30}
if [[ $# -gt 2 || ! "$mode" =~ ^([1-9][0-9]*|x|--compile-only)$ ]]; then
  echo 'Uzycie: test-mwnd.sh [plik.rql] [dodatnia liczba iteracji | x | --compile-only]' >&2
  exit 2
fi

# Rozwiaz sciezki do binarek przed zmiana katalogu roboczego.
xretractor=$(command -v "${XRETRACTOR:-xretractor}")
[[ "$xretractor" = /* ]] || xretractor="$PWD/$xretractor"
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/rdb-mwnd.XXXXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT
shopt -s nullglob
cp -- "$query_dir"/*.txt "$query_dir"/*.dat "$query_dir/$query_name" "$work_dir/"
mkdir "$work_dir/.config"
export XDG_CONFIG_HOME="$work_dir/.config"
export RDB_NAMESPACE=${RDB_NAMESPACE:-mwd$$}

if [[ "$mode" = x || "$mode" = --compile-only ]]; then
  cd -- "$work_dir"
  "$xretractor" -c -q "$query_name"
  exit 0
fi
xtrdb=$(command -v "${XTRDB:-xtrdb}")
[[ "$xtrdb" = /* ]] || xtrdb="$PWD/$xtrdb"
cd -- "$work_dir"
"$xretractor" -c -q "$query_name"
"$xretractor" -q "$query_name" -f -m "$mode" -k

# Przyklady maja po jednej instrukcji SELECT w wierszu. Pokaz tylko strumienie
# wynikowe, ktore rzeczywiscie deklaruje wybrany plik, po osiem rekordow.
{
  while IFS= read -r stream; do
    printf 'echo %s\nopen %s\nsize\nlist 8\n' "$stream" "$stream"
  done < <(awk '$1 == "SELECT" {for (i=2; i<NF; i++) if ($i == "STREAM") {print $(i+1); break}}' "$query_name")
  printf 'quit\n'
} | "$xtrdb" --noprompt
