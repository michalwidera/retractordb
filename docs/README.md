# RetractorDB documentation

Long-lived design documentation for the work that turns RetractorDB from a service
into something a host process can embed. Working notes and roadmap drafts live in
`design/`, which is deliberately untracked; anything that outlives a single revision
belongs here.

| Document | What it answers |
|---|---|
| [`jupyter-integration.md`](jupyter-integration.md) | What the Python binding does today, exactly how far it goes, and what each later phase adds. |
| [`build-options.md`](build-options.md) | The build flags that select an embedded target, and which host platform each one needs. |
| [`core-phase-2.md`](core-phase-2.md) | Where engine state lived, who owns it now (Memory Store, `Engine`), and the gate that keeps the embeddable layer free of process state. |
| [`core-phase-3.md`](core-phase-3.md) | `Engine::compile()` / `step()`: how a host drives the plan one slot at a time, the `retractorcore` split that keeps the service out of it, and what the binding built on it. |

Build commands, testing, code style and the commit/CI policy are not here - they are
in `CLAUDE.md` at the repository root, which remains the binding source for all of it.
