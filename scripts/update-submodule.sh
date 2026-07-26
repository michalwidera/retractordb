#!/usr/bin/env bash

# Aktualizuje wskaźnik (gitlink) podmodułu w repozytorium nadrzędnym: przenosi
# zapisany w nim commit na aktualny HEAD podmodułu i wystawia zmianę do indeksu.
#
# Commit w repozytorium nadrzędnym NIE jest tworzony — na gałęzi master robi to
# człowiek po przejrzeniu diffu (CLAUDE.md, "Commits, push and CI").
#
# Użycie:
#   scripts/update-submodule.sh [--remote] [ścieżka_podmodułu]
#
#   --remote   przed aktualizacją wskaźnika przestaw HEAD podmodułu na gałąź
#              zdalną (fetch + merge --ff-only); bez tej opcji brany jest
#              obecny HEAD podmodułu
#
# Kody wyjścia: 2 błąd użycia, 3 podmoduł ma niezatwierdzone zmiany,
#               4 nie da się przestawić fast-forward

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$script_dir/.." rev-parse --show-toplevel)"

sub_path="examples/experiment"
do_remote=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --remote) do_remote=1 ;;
    -h|--help) sed -n '3,18p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    -*) printf 'Unknown option: %s\n' "$1" >&2; exit 2 ;;
    *) sub_path="$1" ;;
  esac
  shift
done

sub_full="$repo_root/$sub_path"

# Podmoduł to wpis indeksu o trybie 160000 (gitlink), nie zwykły katalog.
if [[ "$(git -C "$repo_root" ls-files -s -- "$sub_path" | awk '{print $1; exit}')" != "160000" ]]; then
  printf 'Not a submodule of %s: %s\n' "$repo_root" "$sub_path" >&2
  printf 'Known submodules:\n' >&2
  git -C "$repo_root" config -f .gitmodules --get-regexp '^submodule\..*\.path$' | awk '{print "  " $2}' >&2
  exit 2
fi

if [[ ! -e "$sub_full/.git" ]]; then
  printf 'Submodule not initialized: %s\n' "$sub_full" >&2
  printf 'Run: git -C %s submodule update --init -- %s\n' "$repo_root" "$sub_path" >&2
  exit 2
fi

recorded="$(git -C "$repo_root" rev-parse "HEAD:$sub_path")"
branch="$(git -C "$sub_full" rev-parse --abbrev-ref HEAD)"

printf -- '-- Submodule:  %s (branch %s)\n' "$sub_path" "$branch"
printf -- '-- Recorded in parent HEAD: %s\n' "$recorded"
printf -- '-- Submodule HEAD:          %s\n' "$(git -C "$sub_full" rev-parse HEAD)"

# Wskaźnik musi wskazywać commit zawierający całą pracę. Gdy drzewo podmodułu
# jest brudne, zapisalibyśmy commit bez niezatwierdzonych zmian — cicha strata.
if [[ -n "$(git -C "$sub_full" status --porcelain)" ]]; then
  printf -- '-- Submodule has uncommitted changes:\n' >&2
  git -C "$sub_full" status --short >&2
  printf 'Commit them inside %s first, then re-run.\n' "$sub_path" >&2
  exit 3
fi

if [[ "$do_remote" -eq 1 ]]; then
  # Gałąź śledzona: z .gitmodules, jeśli zadeklarowana; inaczej obecna.
  sub_name="$(git -C "$repo_root" config -f .gitmodules --get-regexp '^submodule\..*\.path$' |
    awk -v p="$sub_path" '$2 == p { sub(/^submodule\./, "", $1); sub(/\.path$/, "", $1); print $1 }')"
  track="$(git -C "$repo_root" config -f .gitmodules --get "submodule.${sub_name}.branch" || echo "$branch")"

  printf -- '-- Fetching origin/%s...\n' "$track"
  git -C "$sub_full" fetch --quiet origin "$track"
  if ! git -C "$sub_full" merge --ff-only --quiet "origin/$track"; then
    printf 'Cannot fast-forward %s to origin/%s — resolve inside the submodule.\n' "$sub_path" "$track" >&2
    exit 4
  fi
  printf -- '-- Submodule HEAD after fetch: %s\n' "$(git -C "$sub_full" rev-parse HEAD)"
fi

head_sha="$(git -C "$sub_full" rev-parse HEAD)"

if [[ "$head_sha" == "$recorded" ]]; then
  printf -- '-- Pointer already up to date, nothing to stage.\n'
  exit 0
fi

# Commit nieobecny na żadnym zdalnym ref oznacza wskaźnik, którego nikt inny nie
# pobierze. Ostrzeżenie, nie błąd — praca lokalna jest dopuszczalnym stanem.
if [[ -z "$(git -C "$sub_full" branch -r --contains "$head_sha" 2>/dev/null)" ]]; then
  printf -- '-- WARNING: %s is not on any remote-tracking branch of the submodule.\n' "${head_sha:0:12}"
  printf -- '--          Push the submodule before committing the parent pointer.\n'
fi

git -C "$repo_root" add -- "$sub_path"

printf -- '-- Staged pointer move %s -> %s\n' "${recorded:0:12}" "${head_sha:0:12}"
printf -- '-- Parent repository status:\n'
git -C "$repo_root" status --short -- "$sub_path"
printf -- '-- Not committed. Review and commit yourself, e.g.:\n'
printf '     git -C %s commit -m "%s: aktualizacja wskaznika podmodulu"\n' "$repo_root" "$sub_path"
