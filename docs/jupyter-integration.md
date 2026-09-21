# Python, Jupyter and PyTorch integration

**Stage 1a, implemented.** The rest of this document states what is not implemented
and what each remaining phase needs, so the boundary is unambiguous.

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

## 3. The limitation you will hit first - now mostly gone

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

**What still kills the interpreter:** the 60 remaining `FatalError` sites, all on the
**read and write** path - `storage::read`/`revRead`/`write`, `payload`, `fagrp`,
`facc*`, `convertTypes`. `std::exit` is not an exception and does not unwind, so no
`catch` in the binding and no `except` in Python can see them. The binding's guards
for index range and declared sources are still the only protection in front of those.
The guards at the `Storage` constructor are now belt-and-braces: they give Python a
`ValueError` where a `ValueError` is idiomatic, but the engine refuses on its own.

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
| **J1** | `Engine`: `compile()`, `step()`, `rows()`, real exception mapping | core phases 1-3 (phase 1 slice 1 done) |
| **J2** | `Window`, DLPack zero-copy export, `torch.utils.data.IterableDataset` | J1 |
| **J3** | `KeyboardInterrupt` during `run()`, logging bridge to the `logging` module | J1 |
| **J4** | `pyproject.toml` via scikit-build-core, `cibuildwheel`, manylinux wheels | J2 |
| **J5** | Colab notebook, CI smoke test against the built wheel | J4 |
| **J6** | Push ingest from Python, which turns a notebook into the engine's test harness | core phase 4 |

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
