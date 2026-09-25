# Python, Jupyter and PyTorch integration

**Stage 1a implemented; J1, J2 and J3 implemented on top of core phase 3** (see
[`core-phase-3.md`](core-phase-3.md) and §7 below). The rest of this document states what
each stage delivers and what is still not implemented, so the boundary is unambiguous.

The full design, including the PyTorch and Colab reasoning this summarises, is in
`design/Jupyter-retractorDB-roadmap.md` (untracked working notes).

## 1. What exists today

A nanobind extension module, `retractordb._core`, bound directly against the storage
library (`src/rdb/lib`, linked as the `rdb` static target). It is read-only and it
covers the storage layer only - no query plan, no RQL, no execution.

```python
import retractordb as rdb

desc = rdb.load_descriptor("test_db.desc")
print(desc.size_bytes, [f.name for f in desc])

with rdb.Storage("test_db", "test_db", storage_param="/path/to/dir") as st:
    print(len(st))
    print(st[0])          # Record, fields materialised on access
```

| Binding | C++ |
|---|---|
| `FieldType` | `rdb::descFld` |
| `Field` | `rdb::rField` |
| `Descriptor` | `rdb::Descriptor` |
| `load_descriptor(path)` | `rdb::loadDescriptorFile` |
| `Storage` | `rdb::storage` |
| `Record` | `rdb::payload`, read through `getItemVT` |
| `RetractorDBError`, `NoSuchStream`, `StorageError` | raised by the binding's guards |
| `CorruptDescriptor` | `rdb::CorruptDescriptor` - `loadDescriptorFile` |
| `ConfigError` | `rdb::ConfigError` - `storagePaths`, `accessorFactory`, `attachDescriptor` |
| `InternalError` | `rdb::LogicError` - a broken engine invariant; report it, do not handle it |
| `IOError` | `rdb::IOError` - a file operation failed; message carries `strerror(errno)` |

Reads release the GIL. That is not premature: without it a single blocking read
freezes the whole kernel including its UI, and retrofitting the guard after callers
exist means auditing every one of them.

## 2. Why the storage layer and nothing more

Everything above the storage layer needs the shared refactor described in
[`embedded-roadmap.md`](embedded-roadmap.md) §1. `qTree`, `dataModel` and
`executorsm` are service components: they end the process on error, keep
process-wide state, and drive themselves from a wall clock. Binding them today would
produce an API that works once per interpreter and crashes the kernel on any bad
input.

The storage layer needs none of it. Its 25 `.cc` files have no service dependencies,
so this stage is a genuine capability that also answers the two questions the later
stages would otherwise have to guess at: whether nanobind is the right tool, and how
the C++ types actually cross into Python.

## 3. The limitation you will hit first - gone

**A malformed descriptor used to kill the interpreter.** It no longer does:
[core phase 1](core-phase-1.md) slice 1 converted the descriptor read path, so
`load_descriptor` on an empty or unparsable `.desc` raises `CorruptDescriptor` and
the kernel lives.

```python
try:
    desc = rdb.load_descriptor("broken.desc")
except rdb.CorruptDescriptor as err:
    print(err)          # invalid descriptor in file: broken.desc
```

`CorruptDescriptor` is the first type in the hierarchy raised by the **engine**
rather than by a guard in the binding, and it is where the rest of the taxonomy will
attach.

**Sub-slice 2a then closed the path that builds a storage.** An unknown
`storage_type`, a `storage_param` that is not a directory, a descriptor with no REF
field - all `ConfigError` now:

```python
rdb.Storage("s", "s", storage_param=".", storage_type="NONSENSE")
# ConfigError: storage: unsupported storage type 'NONSENSE' - expected one of
#              DEFAULT, DIRECT, MEMORY, POSIX, POSIXSHD, GENERIC, DEVICE, TEXTSOURCE
```

That one is worth singling out: `makeAccessor` is the only place the list of accepted
types exists, so no guard in `module.cpp` could have stood in front of it without
duplicating the list and drifting from it. It killed the kernel and nothing in the
binding could have stopped it.

**Sub-slice 2b finished the job.** `src/rdb` - the whole storage layer plus the `xtrdb`
tool - now contains **zero** `FatalError` call sites. Every failure below the binding is
a throw, so nothing this module can reach will end the interpreter:

```python
desc.field_index("typo")        # KeyError, not a dead kernel
storage[9999]                   # IndexError
rdb.Storage("s", "s", storage_type="NONSENSE")   # ConfigError
```

A new `IOError` covers failures that are neither bad input nor engine bugs - a failed
open, a rejected read - and carries `strerror(errno)` rather than a return code that
was always -1.

The guards in `module.cpp` have changed role rather than disappeared. They are no
longer the only thing standing between a notebook and `std::exit`; they translate the
engine's `ConfigError` / `InternalError` into the types a Python caller expects
(`ValueError`, `KeyError`, `IndexError`).

**What is left, in phase 1: nothing.** Phase 1 is done - the engine no longer contains a
single `FatalError` call site, only the `RDB_FAULT_FATAL_IN_SLOT` diagnostic hook that the
exit-path test uses. `std::exit` is no longer reachable from any engine failure, which is the
whole precondition J1 was waiting on.

Two consequences matter to a notebook even before `Engine` exists. A bad command no longer
ends the service: it answers the client and the engine keeps running (slice C). And every
failure now arrives as an `rdb::Error` subclass, so the binding maps it to a Python
exception instead of guarding a path that would otherwise take the interpreter down with it.

`api/python/tests/test_fatal_paths.py` tracks the boundary in executable form. The two
descriptor cases now assert `pytest.raises` in the test interpreter;
`test_guarded_paths_do_not_end_the_process` still runs in a subprocess and is what
notices a guard being dropped while the site behind it is still there.

`test_reentry.py` does the same for phase 2: it constructs and destroys a `Storage`
100 times in one process, which is the shape that `fatalErrorRaised` and the
`faccmemory.cc` maps will fail under once instances start to overlap. `statusDesc` is
off that list - slice 1 deleted it, and the parser's status is now the state of one
call rather than of the process.

## 3a. nanobind: the measurement stage 1a was for

The roadmap left nanobind versus pybind11 open and said the first binding was the
cheap way to settle it. Measured on macOS arm64, Apple clang, Debug, nanobind 3.1.0,
Python 3.14:

| | |
|---|---|
| `module.cpp` compile | 1.6 s cold, 0.9 s incremental, for 264 lines |
| `_core` module | 8.8 MB, Debug and unstripped |
| Type conversions written by hand | none - `std::variant`, `std::optional`, `std::pair`, `std::string_view` all crossed as they were |

The compile time is the result that matters, because it is what a binding this size
costs on every edit, and it decides how pleasant J1 and J2 are to work on. The module
size proves nothing yet: it statically links an 83 MB Debug `librdb.a`, so it is
measuring the engine, not the binding. Re-measure in Release, stripped, before
quoting a number anywhere.

**Decision: nanobind, confirmed.** Nothing in stage 1a needed a workaround, and the
one API surprise - an object argument rejecting `None` until it is marked `.none()` -
is documented behaviour that cost one line.

## 4. What the later phases add

Phase numbering follows the roadmap in `design/`.

| Phase | Adds | Blocked by |
|---|---|---|
| **J1** | `Engine`: `compile()`, `step()`, `rows()`, real exception mapping | core phases 1-3 - **done**, see §7 |
| **J2** | `Window`, DLPack export, `torch.utils.data.IterableDataset` | J1 - **done** (copy, not zero-copy: §4) |
| **J3** | `KeyboardInterrupt` during `run()`, logging bridge to the `logging` module | J1 - **done** |
| **J4** | `pyproject.toml` via scikit-build-core, `cibuildwheel`, manylinux wheels | J2 - build environment only (`docker/wheel/`, §8); no `pyproject.toml`, no wheels |
| **J5** | Colab notebook, CI smoke test against the built wheel | J4 - a local notebook exists (`api/python/notebooks/j1_engine.ipynb`); the CI smoke test waits for J4 |
| **J6** | Push ingest from Python, which turns a notebook into the engine's test harness | core phase 4 - not started |

Three decisions from that document are worth restating because they constrain the
implementation rather than merely describing it:

**Do not depend on torch.** Export `__dlpack__` and `__dlpack_device__` and let the
user call `torch.from_dlpack` with their own torch. A hard dependency risks pip
reinstalling a 2 GB package in Colab against the wrong CUDA, which is a multi-minute
stall and sometimes a broken runtime. `numpy` is the one reasonable hard dependency.

**Wheels, never a source distribution.** Colab would otherwise compile the engine
plus the ANTLR runtime on every session. The acceptance criterion for J4 is that
`!pip install retractordb` followed by an import takes under about 20 seconds and
requires no runtime restart.

**Copy, do not pin, for the first DLPack implementation.** A `memcpy` of one window
is nanoseconds against a backward pass, and a pinned engine buffer handed to a tensor
that outlives an epoch boundary is a use-after-free with a very long fuse.

## 5. Package layout

One distribution, `retractordb`, with two independent halves:

```
retractordb.client   xqry over shared memory - pure Python, needs a running daemon
retractordb._core    the embedded engine - compiled, needs no daemon
```

`retractordb/__init__.py` imports `_core` lazily, so the client half still imports on
a machine where the extension was never built. Ask for the embedded API on such a
machine and you get an `ImportError` naming the build flag, not an obscure failure.

`retractordb.engine` holds the Python half of `Engine` (`rows()`, `to_numpy()`,
`window()`, `Window`) and needs numpy on first use of the array views; `retractordb.torch`
is the only module that imports torch, and nothing imports it for you.

## 6. Building and testing

See [`build-options.md`](build-options.md). In short:

```bash
scripts/python-venv.sh                    # nanobind + pytest; prints the configure line
cd build/Debug                            # cmake re-runs inside the build directory
cmake -DRDB_PYTHON=ON -DPython_EXECUTABLE="$OLDPWD/.venv-python/bin/python3" .
ninja
cd "$OLDPWD" && .venv-python/bin/python3 -m pytest api/python/tests
```

The suite locates the built module in the build tree by itself and skips cleanly
when there is none, so no installation step is needed and a build without
`RDB_PYTHON` sees no failures.

## 7. What J1-J3 deliver

`rdb::embed::Engine` (L2) compiles a plan and steps it, using the same slot body as the
daemon's loop - `TimeLine` -> the set of streams due in that slot -> `dataModel::processRows`
- without the clock, the IPC thread and the terminal. The plan's storages are built with
the engine's `MemoryStore`, so two engines in one interpreter computing a `VOLATILE` stream
of the same name do not share it; the C++ and Python tests both assert this.

```python
import retractordb as rdb

with rdb.Engine("/tmp/plan") as eng:                     # storage dir for plans without STORAGE
    eng.compile("""
        DECLARE a INTEGER STREAM src, 1/2 FILE '/tmp/plan/data.txt'
        SELECT a*2 STREAM dst FROM src
    """)
    eng.run()                                            # to end of input; Ctrl-C stops it
    print(eng.streams(), eng.slots_done, eng.time)       # ['src', 'dst'] 9 9/2
    print([row[0] for row in eng.rows("dst")])           # [20, 40, ..., 160]

    w = eng.window("dst", fields=["dst_0"], size=4, stride=2, dtype="float32")
    print(w.shape)                                       # (3, 4, 1)
    t = torch.from_dlpack(w)                             # shares the window's memory
```

| Binding | C++ |
|---|---|
| `Engine(storage_dir="")` | `rdb::embed::Engine` |
| `compile(rql, until_eof=True)` | `Engine::compile` - `parsePlanText` + `compiler::compile` + `dataModel` + `TimeLine` |
| `step()` -> `int | None` | `Engine::step`; `None` after the slot in which a declared source ran dry |
| `run(slots=None)` -> `int` | a loop over `step()` in `module.cpp`, GIL released, `PyErr_CheckSignals` every 50 ms |
| `streams()`, `schema()`, `is_declared()`, `record_count()` | the model's `qSet` |
| `record(stream, i)`, `rows(stream)` | `Engine::record` - a copy, oldest first |
| `to_numpy(stream, fields=, dtype=)`, `window(stream, fields=, size=, stride=, dtype=)` | `Engine::project` - `double` per value, `NaN` for null, then one NumPy array per call |
| `Window.__dlpack__` / `__dlpack_device__` / `__array__` | numpy's, on the array the window owns |
| `retractordb.torch.StreamDataset`, `as_tensor` | `torch.from_dlpack` over `window()`; `num_workers=0` |
| `RQLSyntaxError`, `CompileError` | `rdb::embed::SyntaxError`, `rdb::embed::CompileError` - both `ConfigError` |
| engine log records | `logging.getLogger("retractordb")`, through a spdlog sink installed by the module |

**Refused at `compile()`**, as `CompileError`, because the state they need is the daemon's and
not the engine's: `DUMP` and `SYSTEM` rule actions, and `:ROTATION`. `core-phase-3.md` §2.3
has the reasons and §4 what remains owed.

**Not zero-copy.** `window()` materialises `(n_windows, size, n_values)` and `torch.from_dlpack`
shares *that* array; the engine's own memory is never handed out. That is the v1 decision from
§4, taken deliberately, and it is what makes a tensor safe to keep across `step()`.

**J4 has its build environment (§8) and nothing else; J5 and J6 are not started.** No
`scikit-build-core` `pyproject.toml`, no wheels, no CI smoke test, no ingest.
`api/python/pyproject.toml` still installs the client half only.

## 8. J4 build environment

`docker/wheel/Dockerfile` is the image the wheels will be built in. It is
`quay.io/pypa/manylinux_2_28_<arch>` - AlmaLinux 8, glibc 2.28, gcc-toolset-14, CPython
3.10-3.15 and auditwheel - pinned to the digest cibuildwheel 4.x pins, plus what this
tree needs on top: valgrind (the Linux configure requires it), `RDB_USE_MOLD=OFF` (the
Linux default is mold, which the image does not have), a tools venv with conan,
cmake >= 4.4.2 and ninja, and a Release Conan cache for `conanfile.py` under
`CONAN_HOME=/opt/conan`. One file serves x86_64 (Colab) and aarch64; the header has the
build, verification and publishing commands, and the image is published as
`micwide/buildenv-retractordb-wheel:manylinux_2_28_<arch>`.

Why manylinux_2_28: Colab (runtime 2026.07) is Ubuntu 22.04 with glibc 2.35 and Python
3.12, so a 2_28 wheel loads there, and on RHEL 8 and Debian 10/11 as well. manylinux_2_34
has the same GCC 14 but is still marked alpha and excludes those systems.

The image checks two things itself, and a third on demand:

| Check | Where | What a failure means |
|---|---|---|
| C++23 of the engine fits the manylinux_2_28 policy | `toolchain-probe.cpp` + `audit-so.py`, before the Conan step | GCC 14 code using `std::println`, `std::format`, `from_chars(double)` and `std::filesystem` needs libstdc++ symbols up to GLIBCXX_3.4.32; the policy allows 3.4.24. gcc-toolset links the difference statically - if it stops doing so, the build fails in its first minutes, not after Boost |
| The Conan cache is complete | `smoke.sh`: `conan install --build never` | `conanfile.py` changed since the image was built - rebuild it |
| `_core` from this tree is a valid wheel payload | `smoke.sh`: build `_core`, `audit-so.py`, `pytest api/python/tests` | a real portability defect in the engine or the binding |

`audit-so.py` packs the given `.so` files into a throwaway wheel and asks
`auditwheel show` for its tag, which is the same judgement `auditwheel repair` will pass
on the real wheel.

**Both architectures pass end to end** (2026-09-25, Apple silicon, Docker Desktop; x86_64
under emulation). The probe passes on the real base, so gcc-toolset-14 does keep the
engine's C++23 inside the policy; `smoke.sh` finds every Conan package in the cache,
builds `_core` and runs `api/python/tests`:

| | Image build (Conan step) | auditwheel grants | `api/python/tests` |
|---|---|---|---|
| aarch64, native | about 3.5 min (2.3 min) | `manylinux_2_26_aarch64` | 55 passed, 1 skipped |
| x86_64, emulated | about 6 min (5.9 min) | `manylinux_2_27_x86_64` | 55 passed, 1 skipped |

Both tags are at or below the image's `manylinux_2_28`, so a wheel from this image loads
on Colab (glibc 2.35) with room to spare.

Three things the first runs taught, all now handled: the Linux default of mold broke
every configure probe after `PlatformChecks` (hence `RDB_USE_MOLD=OFF` in the image);
ninja's default job count on a 14-core host got `compiler.cpp` at `-O3` killed for lack
of memory, so `smoke.sh` sizes its job count at 2 GiB per job; and the first x86_64 build
failed because Conan Center's prebuilt `b2` for x86_64 needs glibc 2.34, so the image
builds every Conan package from source (`--build "*"`) and takes no binary from Conan
Center at all.

What J4 still has to add on top: the scikit-build-core `pyproject.toml`, the cibuildwheel
configuration that runs `conan install` before the build and points CMake at its
toolchain, the same memory-based cap on build jobs there (`CMAKE_BUILD_PARALLEL_LEVEL`),
a way to configure without valgrind for wheel builds, pushing the image, and the CI job.
