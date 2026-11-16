# it_service_reset_double - the double plan reload

Moved from `test/CLAUDE.md` on 2026-11-16.

`it_service_reset_double` guards the bus reservation against two reloads overlapping in time. A bus slot holds **exactly one** reservation; `resetCommit` took it at *accept* time, while the request flag was cleared at the *start* of `applyPendingPlan`, ~60 lines before `activateReservedPlan`. A reload accepted inside that window overwrote the reservation of the plan then being installed: the outgoing plan activated a foreign reservation (advertising streams it does not compute) and the next epoch found nothing to activate - `FatalError`, which under `--service` also clears the query file, so the unit came back **with no plan**. Two operators running `xqry --reset` at once were enough.

Racing it by hand cost a stagger sweep (one hit in fourteen), so the window is opened by the `RDB_FAULT_PLAN_SWAP_DELAY` hook, the same route as `RDB_FAULT_GET_AWAIT_EPOCH_SWAP`. The test checks four things, because the fix has two sides that break separately: a reload sent before the swap starts is refused, one sent **inside** the window is refused (the regression proper), the server survives and the accepted plan really took over, and - the mirror defect - a reload sent **after** the swap is accepted again. Without that last check, a "refuse always" fix would pass.
