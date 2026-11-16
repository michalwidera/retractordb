# it_xqrywait_gate - the `--xqrywait` gate

Moved from `test/CLAUDE.md` on 2026-11-16.

`it_xqrywait_gate` guards two properties of the `-x` gate, both broken until 2026-09-04 and both fixed by the same change (the gate now waits on its own latch instead of borrowing `iLoopLimitCnt`).

- **A command from the startup window must lift the gate.** The lock file's `PID:` line - the `server_start` contract - is published well before `dataModel` is built, and the gate flag was set only after. The window measured ~90 ms for a 120-stream plan, and a command landing in it was lost: the server waited for the *next* one. Reproduced 5/5.
- **`--llimitqry` must survive the gate.** The gate flag lived in the slot-budget counter, so lifting it wrote back "unbounded": `xretractor -m 5` exited on its own, `xretractor -x -m 5` ran forever. Deterministic, no race involved.

The observable for both is the same and unambiguous: with `-m 5` a server whose gate was lifted exits by itself. Do **not** assert on the server's stdout instead - redirected to a file it is block-buffered, and a grep will report a gate that is merely unflushed as a gate that is stuck. That mistake produced a false 5/5 during this investigation before it was caught.

Part A is timing-assisted: it can pass falsely if the window closes before `xqry` starts, but it cannot fail falsely, so it is repeated three times.

A third property was added on 2026-09-06: **a stop signal must lift the gate too.** The wait was untimed and watched only the first-command latch, while `handleSignal` sets nothing but the loop counter (`notify_all` is not async-signal-safe), so nothing could interrupt it: `xretractor -x` that never received a command survived `SIGTERM` and needed `SIGKILL` - systemd waited out the whole `TimeoutStopSec`. The gate now waits in 100 ms steps and also watches `stop_now`. The test sends **no** command at all, and additionally asserts the storage file stays empty: a server stopped at the gate must not compute the ZERO-step record nobody asked for.
