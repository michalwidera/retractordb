# Build flags for embedded targets

Every embedded consumer of RetractorDB is opt-in, off by default, and selected by one
CMake flag. This page states the convention, so that the iOS and Android flags land
in the shape the Python one already has rather than inventing their own.

## The convention

| Flag | Produces | Host platform | Default | State |
|---|---|---|---|---|
| `RDB_PYTHON` | `retractordb._core`, a nanobind extension module | Linux, macOS | `OFF` | **implemented** |
| `RDB_IOS` | an XCFramework for iOS and iPadOS | macOS only | `OFF` | reserved |
| `RDB_ANDROID` | an AAR for Android | Linux only | `OFF` | reserved |

Four rules hold for all of them:

1. **Off by default.** With the flag unset, nothing about the existing build changes -
   no new targets, no new dependency lookups, no change to any installed artifact.
   A developer who does not care about embedding must not pay for it.
2. **The flag names the consumer, the host platform is a consequence.** `RDB_IOS` is
   not "build for Apple"; it is "build the iOS package", which happens to require
   Xcode and therefore macOS. The flag does not silently select a different host.
3. **A flag on the wrong host is a configuration error, not a warning.** Requesting
   `RDB_ANDROID` on macOS stops the configure step with a message naming the host it
   needs. Silently ignoring an explicitly requested flag produces a green build with
   no artifact, which is the worst of the available outcomes.
4. **Dependencies of an embedded target are looked up only when its flag is on.**
   `find_package(Python3)` must not run in a build that did not ask for Python.

## `RDB_NO_EXCEPTIONS`

Not an embedded target, but a property every embedded target relies on, so it is recorded
here. Default **`ON`**.

With it on, the engine core - the `rdb`, `retractorcore` and `rdbembed` libraries - compiles
with `-fno-exceptions` (and `SPDLOG_NO_EXCEPTIONS`). Errors are `rdb::Result` values
(`src/include/rdb/error.hpp`), and a `throw`, `try` or `catch` in the core is a compile error.
Three files keep exceptions because they wrap third-party code that reports errors only by
throwing: the two ANTLR parser wrappers, `src/rdb/lib/DESCParser.cc` and
`src/retractor/lib/RQLParser.cpp`, and the Boost.Stacktrace wrapper, `src/rdb/lib/stackTrace.cc`
(Boost's default stack-trace backend, which Conan's Boost uses on macOS, has `throw` in its
headers). Each catches everything at its own boundary. The daemon service (`retractor`), the tools and the Python
module keep exceptions, because Boost.Interprocess, `program_options` and nanobind need them.

`OFF` builds the same sources with exceptions enabled. Nothing in the code depends on the flag,
and `test/embedding_boundary.py` rejects `throw`/`try`/`catch` in the core either way. The flag
is deliberately not printed by `xretractor --build-info`, because the H9 corpus checks that
output's set of keys exactly.

One side effect shows up only under `RDB_SANITIZE=address` with libc++ (the macOS
`--sanitize` run). libc++ tags its inline functions with the exception mode, so a unit-test
executable ends up with some `std::vector` code from the instrumented core and some from
Conan's GoogleTest, which is built without sanitizers. AddressSanitizer's container-overflow
check is only reliable when all code is instrumented, and here it reported a false
heap-buffer-overflow while GoogleTest was registering tests. The unit-test `main`
(`test/UnitTest/launch/main.cpp`) therefore sets `detect_container_overflow=0` as its ASan
default. The other ASan checks stay on, and `ASAN_OPTIONS=detect_container_overflow=1` turns
this one back on.

The design, the residual risks and the remaining real-time gaps are in
[`embedded-realtime-gaps.md`](embedded-realtime-gaps.md).

## `RDB_PYTHON`

Builds `retractordb._core` from `src/python/` against the engine core (`rdbembed`, which links
`retractorcore` and `rdb`).

Requirements, all checked at configure time:

| Need | Why |
|---|---|
| Python 3.10+ with development headers | the module links against `Python.h` |
| `nanobind` importable by that interpreter | CMake reads its package directory from the interpreter itself |
| Linux or macOS host | there is no Windows extension build yet, and the flag says so rather than failing at link time |

### Getting an interpreter that has nanobind

A Homebrew or distribution Python refuses `pip install` outright - PEP 668 marks it
externally managed, and `--break-system-packages` is a good way to break the Homebrew
installation it warns you about. So the build does not install anything into the
system Python; it is pointed at an environment that already has what it needs.

```bash
scripts/python-venv.sh
```

That creates `.venv-python/` in the repository, installs what the build and the
checks need into it, runs the same `-m nanobind --cmake_dir` probe CMake will run,
and prints the configure line to paste.

| Package | For |
|---|---|
| `nanobind>=2.0` | building `retractordb._core` |
| `numpy>=1.23` | `Engine.to_numpy()` / `window()` - the embedded half's only runtime dependency |
| `pytest>=7.0` | `api/python/tests` |
| `clang-format==21.1.7`, `cmakelang` | `ninja cformat` |

torch is deliberately absent: `retractordb.torch` imports it on demand and the one test
that needs it skips without it (`jupyter-integration.md` §4).

The formatters are here for the same reason as the rest: `src/CMakeLists.txt` tells
you to `pip install clang-format cmakelang`, and PEP 668 no longer allows that into a
Homebrew Python. clang-format is pinned to the version `CLAUDE.md` names, because
successive releases format differently and an unpinned one means `ninja cformat`
produces a different tree for each developer.

`ninja cformat` invokes bare `clang-format` and `cmake-format`, so it needs this
environment on PATH:

```bash
cd build/Debug
PATH="$OLDPWD/.venv-python/bin:$PATH" ninja cformat
```
 It is idempotent - re-running it on an existing environment
only refreshes the packages. `--python python3.12` picks an interpreter, `--venv DIR`
puts the environment elsewhere, `--recreate` starts over, and `--print-python` emits
just the interpreter path for scripting.

Then point an existing build directory at that interpreter. Re-run cmake **inside**
the build directory - `cmake .` at the repository root would mean an in-source
configure, which this tree does not support because conan owns the build directory
and its toolchain file:

```bash
cd build/Debug
cmake -DRDB_PYTHON=ON -DPython_EXECUTABLE="$OLDPWD/.venv-python/bin/python3" .
ninja
```

With no build directory yet, `scripts/buildrdb.sh conan ninja debug` creates one
first. The venv script prints the exact configure line for every build directory it
finds, so there is nothing to substitute by hand.

`Python_EXECUTABLE` is cached, so later builds in that directory need nothing
further. An activated virtual environment also works - CMake's `FindPython` prefers
one - but passing the path explicitly is what makes the configuration reproducible
from a fresh shell, from an IDE, and from CI.

The built module is written to `${CMAKE_BINARY_DIR}/python/retractordb/`, alongside a
copy of the package's `.py` sources, so `${CMAKE_BINARY_DIR}/python` is a complete
importable root. CMake records it in the cache as `RDB_PYTHON_MODULE_DIR`.

The test suite finds that root by itself - it looks for `build/*/python` holding a
built `_core` - so tests run against a build tree with no install step. Run them with
the environment's interpreter, since that is the one holding pytest:

```bash
.venv-python/bin/python3 -m pytest api/python/tests
```

With two configurations present, or a build directory somewhere else, name the root
explicitly through the environment:

```bash
RDB_PYTHON_MODULE_DIR=build/Release/python \
  .venv-python/bin/python3 -m pytest api/python/tests
```

With no build found, every test skips with a message saying so. That is deliberate:
a developer who never asked for the Python extension should not see failures from it.

### One side effect to know before measuring

A shared module cannot link static libraries that are not position independent, so
with `RDB_PYTHON=ON` the `rdb`, `descparser`, `common`, `retractorcore`, `rqlparser` and
`rdbembed` targets are built with `POSITION_INDEPENDENT_CODE ON`. Most of those are the same
targets the engine binaries link,
so the flag does not only add a module - it changes how the engine itself is
compiled in that build directory.

On Apple silicon this is a no-op: arm64 Darwin code is position independent anyway.
On Linux x86-64 it means `-fPIC` on the engine, which is not free - global references
go through the GOT.

**So do not take performance measurements from a build directory configured with
`RDB_PYTHON=ON`.** The research gate (`ninja test_gate`), the ablation matrix and any
benchmark belong in a build directory where the flag is off, which is the default.
The alternative - compiling a second, position-independent copy of those three
libraries just for the module - doubles their build time and was not worth it while
stage 1a is the only consumer. Revisit when stage 1b makes the module a shipped
artifact.

### Not in a sanitizer build directory, unless you preload the runtime

With `RDB_SANITIZE=address` (for example the directory `scripts/macos-build.sh --sanitize`
configures), `retractordb._core` links the AddressSanitizer runtime. A Python interpreter
loads the module with `dlopen`, which is too late for that runtime: on macOS the process
aborts during import, on Linux it exits with "ASan runtime does not come first". Under pytest
the reason is captured, so all you see is `Fatal Python error: Aborted` or exit code 1. CMake
warns about this at configure time and prints the exact command for the host.

Use a separate build directory without `RDB_SANITIZE` for the Python module. Or, to run the
Python suite under the sanitizer (which is worth doing - it is the only memory check the
binding gets on macOS), preload the runtime:

```sh
# macOS (Apple clang)
DYLD_INSERT_LIBRARIES="$(clang -print-resource-dir)/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" \
ASAN_OPTIONS=detect_container_overflow=0 \
RDB_PYTHON_MODULE_DIR=build/Debug/python .venv-python/bin/python3 -m pytest api/python/tests

# Linux (GCC): libstdc++ as well, because python itself is a C program
LD_PRELOAD="$(g++ -print-file-name=libasan.so) $(g++ -print-file-name=libstdc++.so)" \
ASAN_OPTIONS=detect_leaks=0 \
RDB_PYTHON_MODULE_DIR=build/Debug/python .venv-python/bin/python3 -m pytest api/python/tests
```

`detect_leaks=0` because the interpreter does not free everything at exit, and LeakSanitizer
would report that. `detect_container_overflow=0` for the reason given under
`RDB_NO_EXCEPTIONS`: the Conan libraries the module links are not instrumented.

### What it does not do

It does not build a wheel. Wheel packaging is phase J4 and needs scikit-build-core
and `cibuildwheel` (see [`jupyter-integration.md`](jupyter-integration.md) §4); the
current `api/python/pyproject.toml` still describes the pure-Python client only.

## Reserved flags

`RDB_IOS` and `RDB_ANDROID` are named here and nowhere else in the tree - there is no
CMake code behind them yet, deliberately. They are recorded so that the first person
to implement one does not have to guess the name, the default, or the host-platform
rule, and so that the three flags read as one family rather than three accidents.
