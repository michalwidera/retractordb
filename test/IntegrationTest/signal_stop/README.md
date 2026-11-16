# it_signal_stop - the signal handler that logged

Moved from `test/CLAUDE.md` on 2026-11-16.

`it_signal_stop` guards #284: `handleSignal` may only store two lock-free atomics - the signal number (`receivedSignal`) and `stop_now` - and `run()` logs `Received SIG..., initiating shutdown...` after the epoch loop. Until 2026-10-04 the handler logged through `SPDLOG_WARN`, which takes the sink mutex: a signal delivered to a thread already inside a log call deadlocked the handler, `stop_now` was never set, and the process survived until `SIGKILL`.

Part A sends `SIGTERM`, `SIGINT` and `SIGHUP` from outside in each mode - idle, the `-x` gate, the slot loop - and requires a self-exit with code 0 (the unchanged contract) plus, outside Release, the log line. It proves stopping works, not that the deadlock is gone: an external `kill` lands in the logging window only by chance. Part B opens that window deterministically through the `RDB_FAULT_SIGNAL_IN_LOG=<signum>` hook, a sink that raises the signal from inside `sink_it_`, i.e. with the sink mutex held. Against the old handler in Debug it hangs every run. In Release `SPDLOG_WARN` is compiled out, so part B passes even without the fix - the defect is visible only in Debug and RelWithDebInfo.
