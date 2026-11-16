---
name: delegation
description: Reguły delegowania do subagentów z .claude/agents - tabela, co wolno zlecić któremu agentowi i na jakim modelu, kształt promptu, czekanie na powiadomienie, czytanie raportu jako dowodu, ponowienia. Wywołaj przed każdym wywołaniem narzędzia Agent.
---

# Delegowanie do subagentów - tabela i reguły prowadzenia

Reguły stałe (delegowanie jest wyjątkiem, co zostaje w głównej sesji, zakaz Haiku, rdzeń silnika, jeden przebieg budowy lub testów naraz na maszynie, lista poleceń zatwierdzonych z góry) są w `CLAUDE.md`, sekcja *Delegation to subagents*. Ten plik zawiera resztę.

Delegate only this:

| Work | Agent | Model |
|---|---|---|
| A check that runs 10 minutes or longer (`ninja test-valgrind`, `ninja test_gate`, the ablation suite, a full `ctest`), in the background | `test-runner` | Sonnet |
| A large, mechanical change under an approved plan (the same edit across many files, a batch of similar tests), outside the engine core | `implementer` | Sonnet |
| A search across the sibling repositories or deep in the git history that would take the main session more than ~5 searches | `scout` | Sonnet |
| A rule-conformance review of a finished diff, when the human asks for one | `reviewer` | Sonnet |
| Settling one stated hypothesis about a hard defect (rare race, flaky test, divergent results), when the human asks for it; one copy per competing hypothesis, run in parallel | `investigator` | Opus |

- The delegation prompt is short: goal, paths, exact commands or acceptance criteria, expected report. The agent's definition already carries its rules - do not repeat them.
- Agents load only at session start. If `.claude/agents/` changed during the session, do the work in place - never emulate an agent by pasting its definition into a `general-purpose` prompt.
- While an agent runs in the background, wait for its notification: do not poll it (`ps`, `kill -0`, reading its output file), and do not run the same check in the main session. A check is either delegated or run in place - never both.
- An agent's report is evidence, not a verdict. Before telling the human that a check passed, read the verbatim lines it returned (ctest summary, exit codes, `cmp` results); a missing line means the check was not run. Reading those lines is the verification - do not repeat the check.
- Agents never commit, push, open pull requests or touch CI, and the embargo in *Comparative measurement* binds them as it binds the main session.
- Files changed by `implementer` count toward the *Planning threshold*, and the main session reads its diff before the handoff.
- A failed agent run is retried at most once, with a corrected prompt; after that the main session does the work itself.
