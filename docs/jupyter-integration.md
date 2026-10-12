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

The classes of #463 as a Python caller meets them (#476):

| Class | Python | State afterwards |
|---|---|---|
| K1 `FatalError` | `retractordb.InternalError` | the module is tainted: every later call into the engine raises `InternalError` naming the first error, until the interpreter restarts; `close()` still works |
| K2 plan refused | `PlanError` (a `ConfigError`, so a `ValueError`): `RQLSyntaxError`, `CompileError`, `CorruptDescriptor` | the instance lives without a plan; fix the input and call `compile()` again |
| K3 request refused | `ConfigError` (a `ValueError`): no plan, a stream or index outside it, an unsupported `storage_type` | the instance lives, unchanged |
| K4 `EnvironmentError` | `retractordb.IOError` (an `OSError`): `errno` is the system's code when the engine had one, `EIO` otherwise; `filename` is set when the call took the path as an argument, otherwise the path is in the message | the plan is dead: every later `step()` raises `IOError` again; what it wrote stays readable; `compile()` starts from scratch |
| K5 bad value in the data | `None` / `NaN` in the result; the engine log record (Debug builds only) goes to `logging` | the instance lives |
| K6 deprecated form | `DeprecationWarning` from `compile()`, one per `DECLARE ... FILE` and per postfix `s.avg` | the instance lives |
| K7 abort | none - the interpreter ends | - |

K1 taints the whole module rather than one instance (decision (a) of #463): the engine's process-wide state is shared by every instance, and after a broken invariant nothing says which of it is still sound. An unknown `storage_type` is checked by the binding against the accessor factory's own predicates, so a typo is a `ConfigError` instead of a K1 that would end the session. K5 has no `warnings.warn`: the engine reports it only through a `SPDLOG_WARN`, which a Release build compiles out.

Known K7 paths: a `FatalError` or `EnvironmentError` inside a `noexcept` function (`payload::operator=(payload &&)`, `~binaryDeviceRO` in Debug) or in a destructor running while another exception unwinds, including `Engine.close()` and the engine's destructor, ends the process (#463). The plan refusals of `Engine` are in §7.

`api/python/tests/test_fatal_paths.py` pins this in executable form: each failure
mode raises in the test's own interpreter, and the suite has to survive them all.
`test_error_classes.py` pins the table above row by row; K1, which taints the module,
runs in a subprocess that also checks the interpreter survives K2, K3, K4 and K1.

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
| **J1** | `Engine`: `compile()`, `step()`, `rows()`, plan errors (`RQLSyntaxError`, `CompileError`) | core phases 2-3 (#446) - **done**, see §7 |
| **J2** | `Window`, DLPack export, `torch.utils.data.IterableDataset` | J1 - **done** (copy, not zero-copy: below) |
| **J3** | `KeyboardInterrupt` during `run()`, logging bridge to the `logging` module | J1 - **done** |
| **J4** | `pyproject.toml` via scikit-build-core, `cibuildwheel`, manylinux wheels | J2 - not started |
| **J5** | Colab notebook, CI smoke test against the built wheel | J4 - not started |
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
scripts/python-venv.sh                    # nanobind + numpy + pytest; prints the configure line
cd build/Debug                            # cmake re-runs inside the build directory
cmake -DRDB_PYTHON=ON -DPython_EXECUTABLE="$OLDPWD/.venv-python/bin/python3" .
ninja
cd "$OLDPWD" && .venv-python/bin/python3 -m pytest api/python/tests
```

The suite locates the built module in the build tree by itself and skips cleanly
when there is none, so no installation step is needed and a build without
`RDB_PYTHON` sees no failures.

## 7. What J1-J3 deliver

`rdb::embed::Engine` compiles a plan and steps it, using the same slot body as the service's loop - `TimeLine` -> the mask of streams due in that slot -> `dataModel::processRows` - without the clock, the IPC thread and the terminal. The plan's storages are built with the engine's Memory Store, so two engines in one interpreter computing a `VOLATILE` stream of the same name do not share it; the C++ and Python tests both assert this. The engine links `retractorcore`, not the service library: no bus, no IPC server ([`core-phase-3.md`](core-phase-3.md) §2.1).

```python
import retractordb as rdb

with rdb.Engine("/tmp/plan") as eng:                     # storage dir for plans without STORAGE
    eng.compile("""
        DECLARE a INTEGER STREAM src, 1/2 TEXTFILE '/tmp/plan/data.txt'
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
| `compile(rql, until_eof=True)` | `Engine::compile` - `parsePlanText` + `compiler::compile` + `checkPlanStorage` + `dataModel` + `TimeLine` |
| `step()` -> `int | None` | `Engine::step`; `None` after the slot in which a declared source ran dry |
| `run(slots=None)` -> `int` | a loop over `step()` in `module.cpp`, GIL released, `PyErr_CheckSignals` every 50 ms |
| `failed` | `Engine::failed` - a slot raised, the plan stopped until the next `compile()` |
| `streams()`, `schema()`, `is_declared()`, `record_count()` | the model's `qSet` |
| `record(stream, i)`, `rows(stream)` | `Engine::record` - a copy, oldest first |
| `to_numpy(stream, fields=, dtype=)`, `window(stream, fields=, size=, stride=, dtype=)` | `Engine::project` - `double` per value, `NaN` for null, then one NumPy array per call |
| `Window.__dlpack__` / `__dlpack_device__` / `__array__` | numpy's, on the array the window owns |
| `retractordb.torch.StreamDataset`, `as_tensor` | `torch.from_dlpack` over `window()`; `num_workers=0` |
| `RQLSyntaxError`, `CompileError` (both `PlanError`) | `rdb::embed::Refused`, stage `Parse` / `Check` |
| `ConfigError` | `rdb::embed::Refused`, stage `Request` (no plan, a stream or index outside it) |
| `DeprecationWarning` from `compile()` | `Engine::warnings` - `deprecatedFileWarnings` and `query::deprecatedNotations` |
| `IOError`, `InternalError` | `rdb::EngineFatal` of class Environment / Internal, or a refusal of class Environment |
| engine log records | `logging.getLogger("retractordb")`, through a spdlog sink installed by the module |

`ConfigError` is also a `ValueError` and `IOError` an `OSError`, so `except ValueError` catches every refusal of the plan or the request and `except OSError` every failure of the environment; the whole hierarchy is in `core-phase-3.md` §3.

**Refused at `compile()`**, as `CompileError`, because the state they need is the service's and not the engine's: `DUMP` and `SYSTEM` rule actions, `:ROTATION` and `DEVICE` sources. `core-phase-3.md` §2.4 has the reasons and §5 what remains owed.

**Not zero-copy.** `window()` materialises `(n_windows, size, n_values)` and `torch.from_dlpack` shares *that* array; the engine's own memory is never handed out. That is the v1 decision from §4, taken deliberately, and it is what makes a tensor safe to keep across `step()`.

**J4-J6 are not started.** No `scikit-build-core` `pyproject.toml`, no wheels, no CI smoke test, no ingest. `api/python/pyproject.toml` still installs the client half only.
