# RetractorDB documentation

Long-lived design documentation for the work that turns RetractorDB from a service
into something a host process can embed. Working notes and roadmap drafts live in
`design/`, which is deliberately untracked; anything that outlives a single revision
belongs here.

| Document | What it answers |
|---|---|
| [`embedded-roadmap.md`](embedded-roadmap.md) | Why the Python binding, iOS and Android are one programme and not three, what the shared blocker is, and which stage the tree is in. |
| [`jupyter-integration.md`](jupyter-integration.md) | What the Python binding does today, exactly how far it goes, and what each later phase adds. |
| [`build-options.md`](build-options.md) | The build flags that select an embedded target, and which host platform each one needs. |
| [`core-phase-1.md`](core-phase-1.md) | The refactor that gates every stage above the storage layer, the precedent to copy, and the first slice specified to the file. |
| [`core-phase-2.md`](core-phase-2.md) | Where engine state lived, who owns it now (`MemoryStore`, `Engine`), and the gate that keeps the embeddable layer free of process state. |
| [`core-phase-3.md`](core-phase-3.md) | `Engine::compile()` / `step()`: how a host drives the plan one slot at a time, what the binding built on it, and what the daemon side still owes. |

Build commands, testing, code style and the commit/CI policy are not here - they are
in `CLAUDE.md` at the repository root, which remains the binding source for all of it.
