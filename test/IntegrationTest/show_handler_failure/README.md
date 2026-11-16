# it_show_handler_failure - a handler that fails silently

Moved from `test/CLAUDE.md` on 2026-11-16.

`it_show_handler_failure` guards the other half of the 2026-09-04 investigation of `it_fncall_runtime_case` (*Failure reports from CI* in `test/CLAUDE.md`). The `show` command writes nothing into its reply even when the subscription **succeeds**, so a swallowed exception produced a reply indistinguishable from success: the client started its producer thread and reported a missing response queue a second later, one step away from the cause. The reply now carries `error.response`, and the client checks it exactly as it already checked the reply to `get`.

A CI race cannot be ordered on demand, so the test forces the failure through the `RDB_FAULT_SHOW` hook read by the handler. Without the fix the test goes red on both counts: the client's log lacks the server's reason and still carries `did not appear after`.
