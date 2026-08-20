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
paper_repo="${4:-${RETRACTORDB_PAPER_REPO:-$workspace_dir/paper-arXiv}}"

report_versioned_repo() {
  local label="$1"
  local repo="$2"

  if ! git -C "$repo" rev-parse --git-dir >/dev/null 2>&1; then
    printf '%s MISSING %s\n' "$label" "$repo"
    return 2
  fi

  local current
  current="$(git -C "$repo" rev-parse HEAD)"
  printf '%s VERSIONED %s\n' "$label" "$current"
}

check_external_repo() {
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

  printf '%s STALE indexed=%s current=%s\n' "$label" "$expected" "$current"
  return 1
}

status=0
report_versioned_repo "code" "$code_repo" || status=1
check_external_repo "docs-pl" "$polish_docs_repo" "c154f5cb803b0dd15152ad747c9c1315f271a5b6" || status=1
check_external_repo "docs-en" "$english_docs_repo" "8d543c8cbf95ab7cdb41049be3b30163e225bf5b" || status=1
check_external_repo "paper" "$paper_repo" "5f8f28dec026ac2e64dc9a4ef6f662578a210803" || status=1
exit "$status"
