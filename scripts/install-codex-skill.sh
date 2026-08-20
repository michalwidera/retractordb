#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$script_dir/.." rev-parse --show-toplevel)"
knowledge_installer="$repo_root/../knowledge-index/scripts/install-skill.sh"

if [[ ! -x "$knowledge_installer" ]]; then
  printf 'Missing knowledge-index installer: %s\n' "$knowledge_installer" >&2
  exit 1
fi

exec "$knowledge_installer" "$@"
