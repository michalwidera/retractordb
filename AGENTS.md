# Repository instructions for coding agents

These instructions apply to the whole RetractorDB repository. They assume the reader knows nothing about the project and contain only what `CLAUDE.md` does not cover.

## Required context

1. Read `CLAUDE.md` in full before working with code, tests, build files, or documentation. It is the single source for build commands, testing, code style, integration-test sync traps, ANTLR4 grammar pitfalls, collaboration rules, commit/push/CI policy, and the AI-watermark hygiene procedure — all mandatory.

2. Use the `retractordb-system` skill from `.agents/skills/retractordb-system` for every RetractorDB task.
3. Run the skill's `scripts/check_freshness.sh` before relying on its indexed notes.
4. Treat the current implementation and generated build configuration as the primary source of truth, followed by integration and unit tests, canonical Polish documentation, derived English documentation, and finally the indexed notes.

## Sharing the RetractorDB skill

The repository copy is discovered automatically while working inside this repository. To make the same version
available from the documentation repositories and derived-artifact workspaces, run:

```bash
scripts/install-codex-skill.sh
```

The installer creates `~/.agents/skills/retractordb-system` as a symbolic link to the repository copy. It is idempotent
and refuses to replace an existing file, directory, or link to a different target.

## Commits, push and CI

Follow *Commits, push and CI* in `CLAUDE.md`. That section is binding here and is the only copy of the policy:
the mandatory AI-watermark check before every commit, the human-only rule for `master`, the limits on
side-branch commits, the prohibition on pushing, opening pull requests, or invoking CI without an explicit
human request, and the session-end handoff requirement all live there.
