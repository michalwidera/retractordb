#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$script_dir/.." rev-parse --show-toplevel)"
skill_source="$repo_root/.agents/skills/retractordb-system"
user_skills_dir="${RETRACTORDB_CODEX_SKILLS_DIR:-$HOME/.agents/skills}"
skill_target="$user_skills_dir/retractordb-system"

if [[ ! -f "$skill_source/SKILL.md" ]]; then
  printf 'Missing repository skill: %s\n' "$skill_source" >&2
  exit 1
fi

mkdir -p "$user_skills_dir"

if [[ -L "$skill_target" ]]; then
  current_target="$(readlink -f "$skill_target")"
  if [[ "$current_target" == "$(readlink -f "$skill_source")" ]]; then
    printf 'RetractorDB skill is already linked: %s -> %s\n' "$skill_target" "$skill_source"
    exit 0
  fi

  printf 'Refusing to replace an existing link: %s -> %s\n' "$skill_target" "$current_target" >&2
  exit 2
fi

if [[ -e "$skill_target" ]]; then
  printf 'Refusing to replace an existing path: %s\n' "$skill_target" >&2
  exit 2
fi

ln -s "$skill_source" "$skill_target"
printf 'Linked RetractorDB skill: %s -> %s\n' "$skill_target" "$skill_source"
