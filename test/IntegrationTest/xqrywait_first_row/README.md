# it_xqrywait_first_row - the lost first row

Moved from `test/CLAUDE.md` on 2026-11-16.

`it_xqrywait_first_row` guards the other end of the `--xqrywait` gate (`../xqrywait_gate/README.md`). The command that *lifts* the gate and the command that *creates the client's response queue* were two different commands: `xqry -s` sent `get` first (to validate the stream name) and subscribed only with the later `show`, while the queue is created server-side inside the `show` handler. Between the two the server was already computing and broadcasting to nobody, and at a 1/8 s tick every 125 ms of that gap is one row lost for good. Locally the gap is milliseconds; on a loaded CI container it exceeded the tick, and on 2026-09-08 `it_null_divide_by_zero` received rows 2-4 (`null, 20, null`) instead of 1-3.

The fix has two halves that break separately, so the test has a part for each:

- **The client subscribes with its first command** - `show` before `get` (`qry.cpp`), and before `detail` on the JSON path (`jsonOutput.cpp`).
- **The gate is lifted by a command that has been *handled*, not merely received** (`ipcServer.cpp`, callback `onCommandHandled`). The reorder alone is not enough: signalling on receipt let the processing loop past the gate before the `show` handler registered the subscriber. `subscribe()` then ran under `plan_epoch_mutex`, the same lock a slot takes, so that residual race was decided on acquiring the lock - nanoseconds, and unorderable by hand. Since #283 `subscribe()` runs outside that lock, and the order still holds because registration finishes before the handler returns.

Neither window can be opened with a clock, so each part widens its own through a fault hook, the same route as `RDB_FAULT_PLAN_SWAP_DELAY`: `RDB_FAULT_GET_AWAIT_EPOCH_SWAP` for the client half and `RDB_FAULT_SHOW_DELAY` (a sleep at the top of `commandProcessor`, *before* the epoch lock is taken - inside it, the sleep would stall the slot and hide the very window under test) for the server half. A timing probe would be worthless here: it would measure whether the window closed in time, not whether it is gone.

What this test does **not** cover: the gate is still lifted by *any* command, including someone else's (`hello` from a neighbouring `xqry -l`) - that is the contract `it_xqrywait_gate` pins. A server whose gate was lifted by another process computes without a subscriber and its rows are lost exactly as before. The fix covers the case where the client is first, which is how the `-x` + `ONESHOT` tests in this tree run (fourteen of them when the fix landed).
