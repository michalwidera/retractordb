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
| `load_descriptor(path)` | `rdb::tryLoadDescriptorFile` |
| `Storage` | `rdb::storage` |
| `Record` | `rdb::payload`, read through `getItemVT` |
| `RetractorDBError` and subclasses | engine failures and refusals - see §3 |

Reads release the GIL. That is not premature: without it a single blocking read
freezes the whole kernel including its UI, and retrofitting the guard after callers
exist means auditing every one of them.

## 2. Why the storage layer and nothing more

Everything above the storage layer needs the core work of #446 (phases 2 and 3).
`qTree`, `dataModel` and `executorsm` keep process-wide state and drive themselves
from a wall clock, and the plan layer is not yet separated from the daemon's IPC.
Binding them today would produce an API that works once per interpreter.

The storage layer needs none of it. Its `.cc` files have no service dependencies,
so this stage is a genuine capability that also answers the two questions the later
stages would otherwise have to guess at: whether nanobind is the right tool, and how
the C++ types actually cross into Python.

## 3. Errors

The engine has two ways to report failure (decision (a) of #463):

- **By value.** A refusal the engine can name - a descriptor that does not parse, a
  descriptor file it cannot open - comes back as an `rdb::Refusal` from
  `tryLoadDescriptorFile` or `storage::attachDescriptorStatus`. The binding turns it
  into `CorruptDescriptor`, or `IOError` when the cause is the environment.
- **By `FatalError` / `EnvironmentError`** (classes K1 and K4). In the daemon these end
  the process. Inside an `rdb::ThrowOnFatalScope` they throw `rdb::EngineFatal`
  instead (`src/include/fatalError.hpp`). Every call the binding makes into the engine
  opens that scope, so K1 reaches Python as `InternalError` and K4 as `IOError`
  rather than killing the interpreter.

Two limits remain. A `FatalError` inside a `noexcept` function, or in a destructor
running while another exception unwinds, still ends the process (#463). And the
complete class table of the embedded API - which refusals become `ConfigError`, what
state the module is in after K1, `OSError` with `errno` - is #476.

`api/python/tests/test_fatal_paths.py` pins this in executable form: each failure
mode raises in the test's own interpreter, and the suite has to survive them all.

`test_reentry.py` does the same for phase 2: it constructs and destroys a `Storage`
100 times in one process, which is the shape that process-wide state such as the
`faccmemory.cc` map will fail under once instances start to overlap.

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
| **J1** | `Engine`: `compile()`, `step()`, `rows()`, plan errors (`RQLSyntaxError`, `CompileError`) | core phases 2-3 (#446) |
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
