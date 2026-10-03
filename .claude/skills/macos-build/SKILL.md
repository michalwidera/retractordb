---
name: macos-build
description: Budowa i testy RetractorDB na macOS (Apple silicon) - scripts/macos-build.sh, brak Valgrinda (zamiast niego build z RDB_SANITIZE), słabszy czas rzeczywisty, launchd zamiast systemd. Wywołaj przy budowie lub testach na macOS i przy czytaniu wyników z Darwina.
---

# macOS - budowa i czytanie wyników

Reguły dla kodu platformowego (`RDB_HAS_*`, zera deklarowane w `RDB_PLATFORM_FALLBACKS`) są w `CLAUDE.md`, sekcja *Build*. Ten plik zawiera resztę.

**macOS** (verified only on Apple silicon with macOS 27 and Apple clang 21; Intel and older releases untested. Xcode 16.3+ CLT and deployment target 14.4+ are the minimum forced by `std::print`, not a verified configuration):
```bash
scripts/macos-build.sh          # one pass: configure + build + install + ctest, log in build/macos-build.log
scripts/macos-build.sh release
scripts/macos-build.sh --sanitize   # -DRDB_SANITIZE=address,undefined
```
`scripts/buildrdb.sh` works the same there, except that `toolchain` installs through Homebrew. Differences to keep in mind when reading results:
- **No Valgrind** on Apple silicon, and there will not be one. `ninja test` runs the binaries directly on every platform; the equivalent of a local `test-valgrind` is a `-DRDB_SANITIZE=address,undefined` build.
- **Real time is weaker by design**: SCHED_FIFO applies to a THREAD, not the process (`sched_setscheduler` does not exist), there are no affinity masks at all, `mlockall` reports ENOSYS, and there is no PREEMPT_RT counterpart. macOS is a development and test platform, not a measurement one - the research gate (`ninja test_gate`) does not change that.
- **The service is launchd, not systemd**: `restartCommand` builds `launchctl kickstart -k`, the unit identity comes from `XPC_SERVICE_NAME`, and the package ships no `.service` unit.
