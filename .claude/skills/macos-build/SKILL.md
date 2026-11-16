---
name: macos-build
description: Budowa i testy RetractorDB na macOS (Apple silicon) - scripts/macos-build.sh, brak Valgrinda (zamiast niego build z RDB_SANITIZE), słabszy czas rzeczywisty, launchd zamiast systemd. Zawiera też reguły kodu platformowego - kanarki, RDB_PLATFORM_FALLBACKS, zakres weryfikacji Darwina. Wywołaj przy budowie lub testach na macOS, przy czytaniu wyników z Darwina oraz przed pisaniem lub przeglądem kodu platformowego, sondy albo gałęzi zapasowej.
---

# macOS - budowa i czytanie wyników

Reguła główna kodu platformowego (gałąź wybiera `RDB_HAS_*`, nigdy nazwa systemu) jest w `CLAUDE.md`, sekcja *Build*. Ten plik zawiera resztę: budowę na macOS i pozostałe reguły kodu platformowego.

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

## Platform code

These rules complete the `RDB_HAS_*` rule in `CLAUDE.md` and bind all platform code and its verification:

- **Exception: canaries.** An OS name may state an expectation, never choose a branch: a check that fails loudly (`#error`, `static_assert`, a failing test, CMake `FATAL_ERROR`) when a probe or a library macro gives a different result than expected on that OS, so that a vanished or renamed macro cannot silently switch the build to a fallback branch and compile its tests out. Use `RDB_OS_LINUX` / `RDB_OS_DARWIN` from `generated/platformConfig.h`, not `__linux__`, and prefer a C++ expression to a preprocessor condition: `static_assert(!RDB_OS_LINUX || RDB_HAS_X, "...")`. In C++ a misspelt or missing constant is a compile error; in `#if` it silently evaluates to 0 (the build has no `-Wundef`), which would switch the canary off. A canary only fails; it never computes, selects or skips anything.
- **A fallback branch only by declaration**: a probe whose 0 selects the weaker branch must have that 0 declared in `RDB_PLATFORM_FALLBACKS` (empty on Linux; on Darwin it holds the zeros measured on Apple silicon); any undeclared 0 stops the configuration. A new fallback branch means a new entry in the list of controlled probes in `cmake/PlatformChecks.cmake`.
- **Darwin is not a verification target for platform probes or the ablation floor.** A change to `cmake/PlatformChecks.cmake` or to the probe placement in `CMakeLists.txt`, and the ablation floor, need no run on macOS: the Linux results decide. The report names the Darwin branch as not verified and states its risk in one sentence; that is not a blocker.
