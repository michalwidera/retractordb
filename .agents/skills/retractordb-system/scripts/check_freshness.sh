#!/usr/bin/env bash
set -u

script_path="$(readlink -f "${BASH_SOURCE[0]}")"
skill_dir="$(cd "$(dirname "$script_path")/.." && pwd -P)"
default_code_repo="$(git -C "$skill_dir" rev-parse --show-toplevel 2>/dev/null || true)"

code_repo="${1:-${RETRACTORDB_CODE_REPO:-$default_code_repo}}"
if [[ -z "$code_repo" ]]; then
  printf '%s\n' 'Cannot discover the RetractorDB repository. Pass its path as the first argument or set RETRACTORDB_CODE_REPO.' >&2
  exit 2
fi

workspace_dir="$(dirname "$code_repo")"
polish_docs_repo="${2:-${RETRACTORDB_DOCS_PL_REPO:-$workspace_dir/dokumentacja-rdb}}"
english_docs_repo="${3:-${RETRACTORDB_DOCS_EN_REPO:-$workspace_dir/documentation-rdb}}"

check_repo() {
  local label="$1"
  local repo="$2"
  local expected="$3"

  if ! git -C "$repo" rev-parse --git-dir >/dev/null 2>&1; then
    printf '%s MISSING %s\n' "$label" "$repo"
    return 2
  fi

  local current
  current="$(git -C "$repo" rev-parse HEAD)"
  if [[ "$current" == "$expected" ]]; then
    printf '%s FRESH %s\n' "$label" "$current"
    return 0
  fi

  local commits_ahead
  if git -C "$repo" merge-base --is-ancestor "$expected" "$current" 2>/dev/null; then
    commits_ahead="$(git -C "$repo" rev-list --count "$expected..$current")"
    if [[ "$commits_ahead" == "1" ]]; then
      printf '%s FRESH %s\n' "$label" "$current"
      return 0
    fi
  fi

  printf '%s STALE indexed=%s current=%s\n' "$label" "$expected" "$current"
  return 1
}

status=0
check_repo "code" "$code_repo" "07baa27e129485b68ad8c91e85e3c2aea60630c2" || status=1
check_repo "docs-pl" "$polish_docs_repo" "a93427137cc0f96b7ee9fbdab43715250b55901b" || status=1
check_repo "docs-en" "$english_docs_repo" "40116d9a32f1eb51d1bd12ad8714992146531969" || status=1
exit "$status"
