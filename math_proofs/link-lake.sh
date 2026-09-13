#!/usr/bin/env bash
# Trzyma katalog roboczy Lake (.lake: Mathlib, Verso, wyniki budowania - kilka GB) poza
# repozytorium, w katalogu domowym, i zostawia w projekcie tylko dowiazanie .lake.
#
# Katalog docelowy jest kluczowany skrotem sciezki projektu, nie wersja Lean: dwa klony albo
# worktree nie nadpisuja sobie pakietow, a `install-lean.sh --upgrade` nie zmienia klucza.
# Skrypt jest idempotentny; wolaja go install-lean.sh, render.sh i gen-oracle.sh.
#
#   ./link-lake.sh
#   RDB_LEAN_CACHE=/inny/katalog ./link-lake.sh
set -euo pipefail

cd "$(dirname "$0")"

root="${RDB_LEAN_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/retractordb-lean}"
project="$(basename "$(pwd -P)")"
target="$root/$project-$(pwd -P | sha256sum | cut -c1-12)/.lake"

if [[ -L .lake ]]; then
  [[ "$(readlink .lake)" == "$target" ]] ||
    printf '==> .lake wskazuje na %s (domyslnie %s) - zostawiam\n' "$(readlink .lake)" "$target"
  exit 0
fi

mkdir -p "$(dirname "$target")"
if [[ -d .lake ]]; then
  [[ -e "$target" ]] && {
    printf 'BLAD: istnieja jednoczesnie katalog .lake i %s; usun jeden z nich recznie\n' "$target" >&2
    exit 1
  }
  mv .lake "$target"
  printf '==> przeniesiono .lake do %s\n' "$target"
else
  mkdir -p "$target"
fi
ln -s "$target" .lake
printf '==> dowiazanie .lake -> %s\n' "$target"
