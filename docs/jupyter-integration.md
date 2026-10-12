# Python, Jupyter and PyTorch integration

**Stage 1a and J1-J3 implemented; Linux wheel builds added in C4, notebooks in C5** (see
[`core-phase-3.md`](core-phase-3.md) and §7 below). The rest of this document states what
each stage delivers and what is still not implemented, so the boundary is unambiguous.

The full design, including the PyTorch and Colab reasoning this summarises, is in
`design/Jupyter-retractorDB-roadmap.md` (untracked working notes).

## 1. What exists today

A nanobind extension module, `retractordb._core`, provides both read-only access
to existing storages and an embedded `Engine` that compiles RQL and executes a
plan one logical time slot at a time. It links `rdbembed`, `retractorcore` and the
storage library `rdb`; it needs no running daemon and includes no bus or IPC
server. The `Engine` API and its limits are described in §7.

The storage reading API remains available independently of a compiled plan.
Opening a storage can rebuild its `.meta` index, so this is offline access: `Storage` checks held `xretractor_service*.lock` files before opening. Real protection needs the service's lock directory: pass `config=` with the service's TOML file (its `paths.lock_dir` is used) or `lock_dir=` directly; the argument wins over the file. Without either, the check looks into TMPDIR and `Storage` reports that guess as `UserWarning` (shown once per call site) and logs the same text at INFO to `logging.getLogger("retractordb")`. A lock directory listed in `[embedded] lenient_lock_dirs` is checked leniently, like `xtrdb`: only a held lock refuses, while an unreadable entry or a missing directory does not. Without that key the list is, by definition, the system temporary directory (TMPDIR, or `/tmp` without it) - the directory the service locks in when `paths.lock_dir` is empty; a shared temporary directory is where a stray entry is most likely, and TMPDIR is only a guess (a service started by systemd usually locks in `/tmp`). The key replaces that default, and any other lock directory is checked strictly. The library reads a configuration file only when `config=` names it. This startup check does not exclude a daemon that starts later or uses another lock directory. `close()` and context exit close the storage and flush its index.

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
| `Engine` | `rdb::embed::Engine` - see §7 |
| `RetractorDBError` and subclasses | engine failures and refusals - see §3 |

Reads release the GIL. That is not premature: without it a single blocking read
freezes the whole kernel including its UI, and retrofitting the guard after callers
exist means auditing every one of them.

## 2. Why the first stage covered storage only

Stage 1a deliberately bound only the storage layer. At that point the plan and
execution layers still needed the core work of #446: separating the engine from
the daemon's IPC and giving each embedded engine its own Memory Store.

That work is now implemented in C2-C3. `Engine` uses `TimeLine` and `dataModel`
directly, without the service's wall clock, IPC thread or terminal, and instances
do not share their Memory Stores. The storage binding established how the C++
types cross into Python and confirmed the choice of nanobind (§3a); it no longer
defines the full scope of `_core`.

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
| **J4** | `pyproject.toml` via scikit-build-core, `cibuildwheel`, manylinux wheels | build tooling in C4 (#492), acceptance on Colab in #447 |
| **J5** | Colab notebook, CI smoke test against the built wheel | notebooks and local smoke in C5 (#493, §10); Colab acceptance remains in #448, CI job pending |
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
cmake -S . -B build/Debug-Python -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/build/Debug/generators/conan_toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Debug -DRDB_PYTHON=ON \
  -DPython_EXECUTABLE="$PWD/.venv-python/bin/python3"
cmake --build build/Debug-Python --target _core
RDB_PYTHON_MODULE_DIR="$PWD/build/Debug-Python/python" \
  .venv-python/bin/python3 -m pytest api/python/tests
```

The suite locates the built module in the build tree by itself and skips cleanly
when there is none, so no installation step is needed and a build without
`RDB_PYTHON` sees no failures. Without `RDB_PYTHON_MODULE_DIR` it takes the first
`build/*/python` in sorted order that holds `_core`; with more than one Python
build that may not be the one just built, hence the explicit variable above.

For the full #451 acceptance, build `_core`, `xretractor` and `xqry`, install torch
in the test environment and also set `RDB_XRETRACTOR` to that build's executable.
With both variables set, missing `_core`, NumPy, torch or the reference executable
fails the run. The wheel-only profile keeps torch optional and skips reference
tests when no executable is available; those skips do not count toward #451.

## 7. What J1-J3 deliver

`rdb::embed::Engine` compiles a plan and steps it, using the same slot body as the service's loop - `TimeLine` -> the mask of streams due in that slot -> `dataModel::processRows` - without the clock, the IPC thread and the terminal. The plan's storages are built with the engine's Memory Store, so two engines in one interpreter computing a `VOLATILE` stream of the same name do not share it; the C++ and Python tests both assert this. The engine links `retractorcore`, not the service library: no bus, no IPC server ([`core-phase-3.md`](core-phase-3.md) §2.1).

```python
import retractordb as rdb

with rdb.Engine() as eng:                               # private storage directory; inputs live elsewhere
    eng.compile("""
        DECLARE a INTEGER STREAM src, 1/2 TEXTFILE '/tmp/plan/data.txt'
        SELECT a*2 STREAM dst FROM src
    """)
    eng.run()                                            # to end of input; Ctrl-C stops it
    print(eng.streams(), eng.slots_done, eng.time)       # ['src', 'dst'] 8 4
    print([row[0] for row in eng.rows("dst")])           # [20, 40, ..., 160]

    w = eng.window("dst", fields=["dst_0"], size=4, stride=2, dtype="float32")
    print(w.shape)                                       # (3, 4, 1)
    t = torch.from_dlpack(w)                             # shares the window's memory
```

| Binding | C++ |
|---|---|
| `Engine(storage_dir="", lock_dir="", config="")` | `rdb::embed::Engine` |
| `storage_dir` | effective plan directory, or the default directory without a plan |
| `compile(rql, until_eof=True)` | `Engine::compile` - parser, compiler, offline and ownership checks, `dataModel`, `TimeLine` |
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

**Storage ownership (#450, variant A).** `Engine()` creates a private temporary directory that survives `close()` until the engine is destroyed. An explicit directory, including a RQL `STORAGE` override, must be empty on first use or carry a valid `.retractordb-embedded` marker. The directory is locked until the plan's storages close; a second engine, including a path alias or another process, gets `ConfigError`. `Storage` takes the same lock in a marked directory until it closes. Source files must live outside the owned directory, and every output must stay inside it; a missing source file is refused at `compile()` with `IOError` (`errno` `ENOENT`), whereas the service gives NULL records (#346, #451). An explicit RQL `DISPOSABLE` still authorizes removal of its source. Recompilation may replace owned descriptors and results; a preflight refusal preserves old artifacts while closing the previous plan. Later I/O failures are not transactional. A plan in the private directory skips the service lock check, since no service can use a directory created by `mkdtemp`; an explicit directory is checked under the same `lock_dir` / `config` contract as `Storage`. The daemon's own behavior is unchanged.

**Refused at `compile()`**, as `CompileError`, because the state they need is the service's and not the engine's: `DUMP` and `SYSTEM` rule actions, `:ROTATION` and `DEVICE` sources. `core-phase-3.md` §2.4 has the reasons and §5 what remains owed.

**Not zero-copy.** `window()` materialises `(n_windows, size, n_values)` and `torch.from_dlpack` shares *that* array; the engine's own memory is never handed out. That is the v1 decision from §4, taken deliberately, and it is what makes a tensor safe to keep across `step()`.

**Array conversion.** Floating dtypes represent NULL as NaN. Integer dtypes reject
NULL, infinity and values whose truncated result is outside the target integer
range with `ValueError`. Finite fractions inside that range truncate toward zero.
`window()` selects overlapping windows with NumPy's `sliding_window_view` and
copies the selected windows into one contiguous array. Arrays and DLPack tensors
remain readable after subsequent steps, `close()` and destruction of the engine.

**Torch devices and workers.** `as_tensor(window, device="cpu")` shares the window's
CPU memory. An explicitly requested different device receives a copy.
`StreamDataset(..., device="cpu")` uses the same rule. MPS rejects float64 before
attempting a transfer; create float32 windows or use `dtype=torch.float32` for the
dataset. Use `DataLoader(..., num_workers=0)`: a worker is refused before reading
the engine. Neither `fork` nor creating a new engine in a worker initializer is
part of this dataset's contract. Physical MPS/CUDA transfers need verification on
the corresponding hardware; CPU tests only verify the MPS float64 guard.

### J1-J3 contract and guarding tests (#451)

Tests below live in `api/python/tests/`. A test name identifies coverage, not a
claim that it was run on every platform; the acceptance report records the actual
build, module path, results and unverified scope.

| API / area | Contract | Guarding test / scope |
|---|---|---|
| RQL, descriptors, I/O | Engine failures use `RetractorDBError` subclasses; K2/K3 remain recoverable. Name/index/array conversion errors use the usual `KeyError`, `IndexError`, `ValueError`; an invalid Python signature can raise `TypeError`. | `test_engine.py`, `test_fatal_paths.py`, `test_error_classes.py`; standard Python exceptions are an explicit exception to the broad wording of #451. |
| Internal failure | K1 becomes `InternalError` and taints the whole module until interpreter restart. `close()` remains available. Failure in a `noexcept` destructor can still terminate the process. | K1 and guarded API paths run in subprocesses in `test_error_classes.py` / `test_fatal_paths.py`; this is not proof of every fatal path. |
| Missing source | `compile()` refuses a declared source file that does not exist with `IOError` (`errno` `ENOENT`, the path in the message); the previous plan is closed and a corrected cell works. The service keeps NULL records (#346), so this case is outside the reference intersection. | `test_engine.py::test_missing_source_is_a_typed_error_unlike_the_service`; C++ `ut_embedEngine` `missing_source_is_refused_unlike_the_service`. A file removed after `compile()` is not covered by this check. |
| Lifecycle | A new compile closes the old plan, even if compilation fails. A slot failure latches `failed` and refuses further steps until a new compile. Ordinary close, context exit and destruction release resources. | Recompile/failure tests in `test_engine.py` and `test_error_classes.py`; `test_isolation.py::test_engine_context_and_destructor_release_owned_storage`. |
| Slots / EOF | `step()` returns a slot index or `None`; `run(slots=...)` has a budget. Default EOF stops at the first exhausted source; `until_eof=False` wraps. | `test_engine.py`; multi-rate/EOF and delayed output in `test_reference.py`. |
| Service features | DUMP, SYSTEM, ROTATION and DEVICE refuse compilation. | Four service-feature cases in `test_engine.py`; these plans are outside the reference intersection. |
| Isolation / bus | MEMORY belongs to each engine. Engine does not join a bus, claim a namespace or create presence/IPC objects, including after SIGKILL; it can run beside service and standalone. | Instance tests in `test_engine.py` / C++ `ut_embedEngine`; six bus snapshots in `test_isolation.py`, on POSIX `/dev/shm`. |
| Retention | An expired record index raises instead of reading another ring-buffer entry. | `test_reference.py` volatile cases and `test_window.py` retained-tail case. |
| NumPy / windows / DLPack | Field order and arrays determine the value axis; `(windows,size,values)` respects size/stride and empty results. Copies survive the engine; DLPack shares the window allocation. | `test_window.py`, including NULL, numeric boundaries and owner destruction. |
| PyTorch | Import remains lazy. Workers refuse before engine access. CPU shares memory; explicit device transfer copies. Float64/MPS refuses with a float32 hint. | `test_window.py`, including a real worker in a bounded subprocess; hardware transfer remains a separate check. |
| Interrupt / logging | `run()` checks Python signals and stays usable after `KeyboardInterrupt`; engine diagnostics reach the retractordb logger. | `test_engine.py` interrupt and logging cases; interrupt is simulated with `_thread.interrupt_main`. |
| Threading | Serialize every call on one Engine, including getters/reads/close/compile while run is active. Compilation, stepping, running and record reads release the GIL; the GIL does not protect engine state. Concurrent compilation or execution of separate engines has no general thread-safety guarantee. | Source contract; the interrupt test verifies another Python thread can interrupt run, not concurrent engine access. No mutex or thread-safety guarantee is added by #451. |

`test_reference.py` compares the same logical plan and inputs in Engine and this
checkout's `xretractor -u`, with separate output directories. It checks schema,
record order/count, values/NULL, metadata and emission times reconstructed from
the reference compiler's interval/origin/tail listing. This checks the Engine
schedule against the reference plan; it is not a trace of standalone wall-clock
execution. The supplemental wrapping case compares N slots with `until_eof=False`
against `xretractor -m N+1`: the CLI counter stops at 1, whereas the Python budget
counts executed slots. This case does not stop at EOF. A missing source is excluded
with its reason: Engine refuses it, while the service gives NULL records.
The intersection includes all five compatible `expr_corpus` plans.
The eight plans with DUMP remain listed with their exclusion reason, and a scope
test fails if an unclassified plan enters the corpus.

The Linux wheel build is described below. J5 (notebook and CI smoke) and J6 (ingest) remain separate stages. `api/python/pyproject.toml` still installs the client half only.

## 8. J4 build environment

`docker/wheel/Dockerfile` extends PyPA's pinned `manylinux_2_28` image with valgrind
(required by the Linux CMake configuration), a tools venv containing Conan, CMake
>= 4.4.2 and Ninja, and a Release Conan cache under `CONAN_HOME=/opt/conan`.
`RDB_USE_MOLD=OFF` selects the linker available in the image. The same Dockerfile
serves x86_64 and aarch64; its header gives the base digests and build commands.

Before building dependencies, `toolchain-probe.cpp` exercises the engine's C++23
library features. `audit-so.py` asks `auditwheel show --json` for the resulting
library's manylinux tag and rejects a glibc requirement above the image's policy
or external libraries outside it. All Conan dependencies are built from source
in that image, including build tools, to avoid importing binaries made for newer
glibc versions.

From the repository root:

```bash
docker build --platform linux/amd64 --build-arg RDB_VERSION="$(cat VERSION)" \
  -f docker/wheel/Dockerfile \
  -t micwide/buildenv-retractordb-wheel:manylinux_2_28_x86_64 .
docker run --rm --platform linux/amd64 -v "$PWD:/src:ro" \
  micwide/buildenv-retractordb-wheel:manylinux_2_28_x86_64 \
  bash /src/docker/wheel/smoke.sh
```

`smoke.sh` copies tracked and new nonignored files into the container, checks the
Conan cache with `--build never`, builds `_core`, audits it and runs
`api/python/tests`. It prints the imported module's path and gives subprocess
tests the same package root. The host checkout stays mounted read-only.
`jobs.sh` caps parallel compilation at one job per 2 GiB, respecting cgroup limits.

The audit also rejects symbols from `bus`, `ipcServer`, `serverName`,
`lockManager` and `shmBudget`. A Release module is stripped, so smoke supplies its
linker map to check linked service objects as well as exported symbols.

## 9. Building the wheels

The root `pyproject.toml` uses scikit-build-core and nanobind. It reads the version
from `VERSION`, configures `RDB_PYTHON=ON`, builds only `_core` and installs only
the `python` component alongside `api/python/retractordb`. NumPy is the runtime
dependency; torch is optional. The separate `api/python/pyproject.toml` remains
the client-only development installation.

Both files declare the same distribution name, `retractordb` - the wheel with the
version from `VERSION`, the client installation with its own fixed version. pip
treats them as one package: `pip install ./api/python` in an environment holding
the wheel uninstalls the wheel, `_core` included, and installing the wheel
replaces the client installation. Keep them in separate venvs.

```bash
pipx install cibuildwheel
docker/wheel/build-wheels.sh x86_64       # or aarch64, or all
```

The script copies the checkout without ignored build artifacts to a temporary
directory and writes wheels to `build/wheelhouse/`. It uses the locally built images
when tagged `micwide/buildenv-retractordb-wheel:manylinux_2_28_<arch>`; publishing
them is a separate human action.

Per architecture the configured builds produce CPython 3.10 and 3.11 wheels plus
one `cp312-abi3` wheel for Python 3.12 and later. `STABLE_ABI` is active when
scikit-build-core requests `Development.SABIModule`; ordinary development builds
keep the interpreter-specific ABI. `EXCLUDE_FROM_ALL` keeps `_core` out of normal
`ninja install`, and the explicit CPack component list excludes `python`.

cibuildwheel repairs the manylinux wheels, runs `abi3audit --strict` on the abi3
wheel and tests each installed wheel. The test command imports `_core` explicitly
and sets `RDB_PYTHON_MODULE_DIR` to its installed package root, so subprocess tests
also exercise the wheel instead of skipping for lack of a build tree.

Colab acceptance remains in #447: install the x86_64 abi3 wheel in a fresh
notebook and check that install plus import takes under about 20 seconds without
a runtime restart. C5 adds the notebooks and local smoke test (§10); wheel CI,
PyPI publication and macOS wheels remain outside C4.

## 10. Notebooks and their smoke test

C5 (#493) adapts three notebooks from `feature/jupyter-integration` to the current
RQL syntax and exception hierarchy (§3). All committed cell outputs are empty.

| Notebook in `api/python/notebooks/` | Flow |
|---|---|
| `j0_storage.ipynb` | descriptor, copied storage, NULL values, negative indexing and `CorruptDescriptor` with a usable kernel afterwards |
| `j1_engine.ipynb` | `TEXTFILE`, compile/step/run, records, NumPy/DLPack windows, plan refusals and recompilation |
| `colab_quickstart.ipynb` | wheel install, noisy signal from `TEXTFILE`, `AVG(v:100)`, windows and a PyTorch training loop |

The Colab notebook checks the moving average against NumPy over the same input.
Its first cell installs with `%pip` and prints the compiled module path and install
time. Until publication on PyPI, upload a Linux wheel through Colab's Files panel
and set `WHEEL` to its path, for example
`'/content/retractordb-*-cp312-abi3-*_x86_64.whl'`. The smoke script supplies it via
`RDB_WHEEL`.

For local work, `RDB_PYTHON_MODULE_DIR` selects the built package root (§6) in all
three notebooks; the interpreter must match the extension. Without it, `j0` and
`j1` take the first `build/*/python` with an extension in sorted order. Open them
from `api/python/notebooks`, or set `RDB_REPO_DIR` to the checkout root. The script
sets this variable when running from a temporary directory, including in a
container with a read-only checkout. `j0` copies only the data, descriptor and
metadata fixtures, omitting the shadow update that would replace the first record.

`docker/wheel/notebook-smoke.sh` runs all three notebooks through `nbclient`, each
in a separate kernel registered for the selected interpreter. It validates their
JSON, rejects deprecated RQL forms, fails on cell errors and checks that every
notebook imported the selected `_core`. It never saves executed cell outputs in
the source tree.

The wheel profile uses Python 3.12, NumPy 2.0.2 and torch 2.11.0 in a fresh venv.
These are fixed local test dependencies; they do not declare the current Colab
runtime versions. Build the wheel with §9 first, then run on Linux:

```bash
docker/wheel/notebook-smoke.sh [wheel]
# Or in a Linux container, with the checkout and build/wheelhouse read-only:
docker run --rm -v "$PWD:/src:ro" python:3.12-slim \
  bash /src/docker/wheel/notebook-smoke.sh
```

With no argument the script requires exactly one host-architecture cp312-abi3
wheel in `build/wheelhouse/`. `PYTHON` selects another Python 3.12 executable. torch
comes from the PyTorch CPU index by default; `RDB_TORCH_INDEX=` uses PyPI instead.
The wheel profile also requires `pip freeze --all` to change only by adding
`retractordb`, and install plus import to take at most 20 seconds (§4).

The build profile uses an existing interpreter with NumPy, torch, `nbclient` and
`ipykernel`; it installs nothing and does not measure wheel installation:

```bash
.venv-python/bin/python3 -m pip install nbclient ipykernel torch
PYTHON="$PWD/.venv-python/bin/python3" \
  docker/wheel/notebook-smoke.sh --module-dir build/Debug-Python/python
```

C5 brings the notebooks and this local check. Acceptance in a real Colab runtime
remains in #448; running locally does not establish the Colab install/restart
criterion. A CI job for the wheel notebook smoke remains pending.
