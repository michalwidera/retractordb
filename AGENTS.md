# Repository instructions for coding agents

These instructions apply to the whole RetractorDB repository.

## Required context

1. Read `CLAUDE.md` in full before working with code, tests, build files, or documentation. Follow its repository rules,
   except for instructions that are specific to Claude model selection.
2. Use the `retractordb-system` skill from `.agents/skills/retractordb-system` for every RetractorDB task.
3. Run the skill's `scripts/check_freshness.sh` before relying on its indexed notes.
4. Treat the current implementation and generated build configuration as the primary source of truth, followed by
   integration and unit tests, canonical Polish documentation, derived English documentation, and finally the indexed
   notes.

## Sharing the RetractorDB skill

The repository copy is discovered automatically while working inside this repository. To make the same version
available from the documentation repositories and derived-artifact workspaces, run:

```bash
scripts/install-codex-skill.sh
```

The installer creates `~/.agents/skills/retractordb-system` as a symbolic link to the repository copy. It is idempotent
and refuses to replace an existing file, directory, or link to a different target.

## Commits, push, and CI

- On `master` in this code repository, commits and pushes are performed by the human only after reviewing the diff.
  Agents leave changes uncommitted and hand them over for review.
- On side branches, agents may create local commits after verification, provided that doing so does not start CI.
- Agents do not push, open pull requests, or invoke CI without an explicit human request.
- If an action would trigger CI, stop and hand it over to the human.
