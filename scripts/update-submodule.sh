#!/usr/bin/env bash

# Podciąga podmoduł do HEAD gałęzi śledzonej i przesuwa na ten commit wskaźnik
# (gitlink) w repozytorium nadrzędnym, wystawiając zmianę do indeksu.
#
# Zasada: wskaźnik idzie WYŁĄCZNIE w górę. Podmoduł ma pokazywać HEAD niezależnie
# od tego, na której gałęzi stoi repozytorium nadrzędne — a różne gałęzie zapisują
# różne, często starsze commity podmodułu. Cofnięcie wskaźnika jest odmawiane
# (kod 5), bo znaczy, że drzewo robocze podmodułu zostało przewinięte wstecz
# (typowo przez 'git submodule update' po zmianie gałęzi nadrzędnego).
#
# UWAGA: gitlink przechowuje wyłącznie SHA commitu — nigdy nazwy gałęzi. Skrypt
# nie aktualizuje więc żadnej gałęzi nadrzędnego. Jedynym miejscem, gdzie nazwa
# gałęzi podmodułu jest zapisana, jest klucz submodule.<nazwa>.branch
# w .gitmodules; skrypt go czyta, ale nie modyfikuje.
#
# Commit w repozytorium nadrzędnym NIE jest tworzony — na gałęzi master robi to
# człowiek po przejrzeniu diffu (CLAUDE.md, "Commits, push and CI").
#
# Użycie:
#   scripts/update-submodule.sh [--no-fetch] [--allow-rewind] [ścieżka_podmodułu]
#
#   --no-fetch      nie sięgaj do origin; użyj obecnego HEAD podmodułu (praca
#                   offline albo commit jeszcze niewypchnięty)
#   --allow-rewind  pozwól cofnąć wskaźnik (świadome przypięcie do starszego
#                   commitu podmodułu)
#
# Kody wyjścia: 2 błąd użycia, 3 podmoduł ma niezatwierdzone zmiany,
#               4 nie da się przestawić fast-forward, 5 wskaźnik cofnąłby się

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$script_dir/.." rev-parse --show-toplevel)"

sub_path="examples/experiment"
do_fetch=1
allow_rewind=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-fetch) do_fetch=0 ;;
    --allow-rewind) allow_rewind=1 ;;
    -h|--help) awk 'NR>1 && /^#/ { sub(/^# ?/, ""); print; next } NR>2 { exit }' "${BASH_SOURCE[0]}"; exit 0 ;;
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
sub_name="$(git -C "$repo_root" config -f .gitmodules --get-regexp '^submodule\..*\.path$' |
  awk -v p="$sub_path" '$2 == p { sub(/^submodule\./, "", $1); sub(/\.path$/, "", $1); print $1 }')"

# Po 'git submodule update' podmoduł stoi w detached HEAD — czyli dokładnie
# w stanie "pokazuję commit zapisany przez gałąź nadrzędnego", a nie HEAD.
if branch="$(git -C "$sub_full" symbolic-ref --quiet --short HEAD)"; then
  branch_label="branch $branch"
else
  branch=""
  branch_label="detached at $(git -C "$sub_full" rev-parse --short=12 HEAD)"
fi

# Gałąź śledzona: .gitmodules ma pierwszeństwo, potem obecna gałąź podmodułu,
# a w detached HEAD — domyślna gałąź origin.
track="$(git -C "$repo_root" config -f .gitmodules --get "submodule.${sub_name}.branch" || echo "$branch")"
if [[ -z "$track" ]]; then
  track="$(git -C "$sub_full" symbolic-ref --quiet --short refs/remotes/origin/HEAD 2>/dev/null || true)"
  track="${track#origin/}"
fi

printf -- '-- Parent branch:           %s\n' "$(git -C "$repo_root" rev-parse --abbrev-ref HEAD)"
printf -- '-- Submodule:               %s (%s)\n' "$sub_path" "$branch_label"
printf -- '-- Tracked branch:          %s\n' "${track:-<unknown>}"
printf -- '-- Recorded in parent HEAD: %s\n' "$recorded"
printf -- '-- Submodule HEAD:          %s\n' "$(git -C "$sub_full" rev-parse HEAD)"

# Blokują wyłącznie ZMIANY W PLIKACH ŚLEDZONYCH: zapisalibyśmy wtedy commit bez
# nich, czyli cicho zgubili pracę. Pliki nieśledzone nie mogą zginąć przy
# przesunięciu wskaźnika, a po cofnięciu podmodułu pojawiają się masowo
# (pliki z nowszych commitów) — blokowanie na nich uniemożliwiłoby naprawę
# dokładnie tego stanu, który skrypt ma prostować. Stąd tylko ostrzeżenie.
if [[ -n "$(git -C "$sub_full" status --porcelain --untracked-files=no)" ]]; then
  printf -- '-- Submodule has uncommitted changes to tracked files:\n' >&2
  git -C "$sub_full" status --short --untracked-files=no >&2
  printf 'Commit them inside %s first, then re-run.\n' "$sub_path" >&2
  exit 3
fi

untracked_count="$(git -C "$sub_full" ls-files --others --exclude-standard | wc -l)"
if [[ "$untracked_count" -gt 0 ]]; then
  printf -- '-- NOTE: %s untracked path(s) in the submodule — not part of any commit:\n' "$untracked_count"
  git -C "$sub_full" ls-files --others --exclude-standard | head -5 | sed 's/^/--        /'
fi

if [[ "$do_fetch" -eq 1 ]]; then
  if [[ -z "$track" ]]; then
    printf 'Cannot determine the tracked branch of %s.\n' "$sub_path" >&2
    printf 'Check out a branch inside it, declare one, or pass --no-fetch:\n' >&2
    printf '  git -C %s config -f .gitmodules submodule.%s.branch <branch>\n' "$repo_root" "$sub_name" >&2
    exit 2
  fi

  printf -- '-- Fetching %s from origin...\n' "$track"
  if ! git -C "$sub_full" fetch --quiet origin "$track"; then
    printf 'No branch %s on the submodule remote (origin).\n' "$track" >&2
    printf 'Push it first, declare the tracked branch in .gitmodules, or pass --no-fetch:\n' >&2
    printf '  git -C %s config -f .gitmodules submodule.%s.branch <branch>\n' "$repo_root" "$sub_name" >&2
    exit 4
  fi

  # Detached HEAD albo inna gałąź niż śledzona: wracamy na śledzoną, żeby podmoduł
  # znów "pokazywał HEAD", a nie commit przypięty przez gałąź nadrzędnego.
  if [[ "$branch" != "$track" ]]; then
    printf -- '-- Checking out %s in the submodule...\n' "$track"
    git -C "$sub_full" checkout --quiet "$track" 2>/dev/null ||
      git -C "$sub_full" checkout --quiet -b "$track" FETCH_HEAD
  fi

  # FETCH_HEAD, nie origin/<gałąź>: dla gałęzi pobranej doraźnie ref śledzący
  # nie musi istnieć, a FETCH_HEAD jest dokładnie tym, co właśnie pobrano.
  if ! git -C "$sub_full" merge --ff-only --quiet FETCH_HEAD; then
    printf 'Cannot fast-forward %s to origin/%s — resolve inside the submodule.\n' "$sub_path" "$track" >&2
    exit 4
  fi
  printf -- '-- Submodule HEAD after pull-up: %s\n' "$(git -C "$sub_full" rev-parse HEAD)"
fi

head_sha="$(git -C "$sub_full" rev-parse HEAD)"

if [[ "$head_sha" == "$recorded" ]]; then
  printf -- '-- Pointer already at submodule HEAD, nothing to stage.\n'
  exit 0
fi

# Wskaźnik idzie tylko w górę: nowy commit musi mieć zapisany jako przodka.
# Inaczej albo cofamy się (przewinięty podmoduł), albo gałęzie się rozjechały.
if ! git -C "$sub_full" merge-base --is-ancestor "$recorded" "$head_sha"; then
  if git -C "$sub_full" merge-base --is-ancestor "$head_sha" "$recorded"; then
    kind="rewind"
  else
    kind="divergence"
  fi
  if [[ "$allow_rewind" -eq 0 ]]; then
    printf -- '-- Refusing pointer %s: %s -> %s\n' "$kind" "${recorded:0:12}" "${head_sha:0:12}" >&2
    printf 'The submodule worktree is not ahead of what this parent branch records.\n' >&2
    printf 'Bring it up first (drop --no-fetch), or pass --allow-rewind if intended.\n' >&2
    exit 5
  fi
  printf -- '-- WARNING: accepting pointer %s: %s -> %s (--allow-rewind).\n' \
    "$kind" "${recorded:0:12}" "${head_sha:0:12}"
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
