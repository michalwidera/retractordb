# Repository instructions for coding agents

These instructions apply to the whole RetractorDB repository. They assume the reader knows nothing about the project and contain only what `CLAUDE.md` does not cover.

## Required context

1. Read `CLAUDE.md` in full before working with code, tests, build files, or documentation. It is the single source for build commands, testing, code style, integration-test sync traps, ANTLR4 grammar pitfalls, collaboration rules, commit/push/CI policy, and the AI-watermark hygiene procedure — all mandatory.

2. Use the `retractordb-system` skill from the sibling `knowledge-index` repository for every RetractorDB task. The
   `.agents/skills/retractordb-system` path in this repository is only a relative symbolic link to that checkout.
3. Run the skill's `scripts/check_freshness.sh` before relying on its indexed notes.
4. Resolve conflicts between sources using the skill's *Source precedence* section, which is the only copy of that
   ordering.

## Installing the RetractorDB skill

The sibling `knowledge-index` checkout is discovered through the repository symlink. To make the same skill available
from the documentation repositories and derived-artifact workspaces, run:

```bash
scripts/install-codex-skill.sh
```

The installer delegates to `../knowledge-index/scripts/install-skill.sh` and creates
`~/.agents/skills/retractordb-system` as a symbolic link to the knowledge-index checkout. It is idempotent and refuses
to replace an existing file, directory, or link to a different target.

## Commits, push and CI

Follow *Commits, push and CI* and *AI watermark hygiene (text)* in `CLAUDE.md`. Both are binding here and hold
the full text: the mandatory watermark check before every commit, the human-only rule for `master`, the limits
on side-branch commits, the prohibition on pushing, opening pull requests, or invoking CI without an explicit
human request, and the session-end handoff requirement. The `retractordb-system` skill restates none of it — it
only adds the deltas that apply to the documentation and paper repositories.
