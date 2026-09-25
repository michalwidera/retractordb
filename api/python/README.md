# Python API

This package carries two independent halves. The **client** below talks to a running
`xretractor` daemon and is pure Python. The **embedded engine**
(`retractordb._core`) runs the engine in-process, needs no daemon, and is built
only on request - see [`docs/build-options.md`](../../docs/build-options.md) and
[`docs/jupyter-integration.md`](../../docs/jupyter-integration.md).

## Client (xqry)

Requires Linux, Python 3.10+, and matching `xqry` / `xretractor` binaries. There are no runtime dependencies outside the Python standard library.

```sh
python3 -m venv .venv-api
.venv-api/bin/python -m pip install ./api/python
.venv-api/bin/python api/python/examples/monitor.py laboratory temperature --limit 10
```

The server must already be running, for example `xretractor query.rql --name laboratory -k` in another terminal. Create the query's storage directory first when using `STORAGE`.

```python
from retractordb import Client, Error, ReadTimeout

with Client("laboratory", xqry="/path/to/xqry") as db:
    print(db.streams())
    print(db.describe("temperature"))
    with db.subscribe("temperature", limit=10) as samples:
        for record in samples:
            print(record["v"])
        print(samples.end_reason)
```

`Client(server, *, xqry="xqry", timeout=5.0)` uses seconds for timeouts. `subscribe(stream, *, limit=0, idle_timeout=0.0, capacity=1024)` starts an independent child. A zero limit means unlimited records. `samples.schema` is available when `subscribe()` returns. `samples.pid`, `samples.returncode` and `samples.closed` expose process/lifecycle state.

`samples.next(timeout=0.5)` returns a `Record`, returns `None` at normal end, or raises `ReadTimeout` while keeping the subscription open. Iteration uses an unlimited read wait. `Error.code` identifies other failures. Use `close()` or leave the context to stop. Breaking a loop alone does not close its iterator; keep the surrounding `with`.

`Record.values` maps field names to scalars or lists for arrays with more than one element. NULL is `None`, including inside arrays. Rationals and stream periods use exact `Fraction` objects. `Schema.fields` retains field order.

```python
with Client("laboratory") as db:
    with db.subscribe("temperature") as a, db.subscribe("pressure") as b:
        print(a.next(timeout=2.0).values)
        print(b.next(timeout=2.0).values)
```

For a GUI, use timed reads from its worker or timer and handle `ReadTimeout`. Serialize calls on each handle. The package does not impose an event loop or invoke callbacks on the GUI thread. The example handles Ctrl+C; applications using other termination signals must arrange context cleanup themselves.

See [the shared contract](../README.md) for delivery and lifecycle limits.

## Embedded engine

Built only with `-DRDB_PYTHON=ON`. Stage 1a covers the storage layer, read-only:
reading `.desc` files and records without a running server. J1-J3 add `Engine`:
a plan compiled and stepped in-process, with NumPy and DLPack views of its streams
(see below).

```sh
scripts/python-venv.sh              # venv with nanobind + pytest; prints the next line
cd build/Debug                      # cmake must re-run INSIDE the build directory
cmake -DRDB_PYTHON=ON -DPython_EXECUTABLE="$OLDPWD/.venv-python/bin/python3" .
ninja
cd "$OLDPWD" && .venv-python/bin/python3 -m pytest api/python/tests
```

The script exists because Homebrew and distribution Pythons refuse `pip install`
under PEP 668. It is idempotent; `scripts/python-venv.sh --help` lists the options.

```python
import retractordb as rdb

desc = rdb.load_descriptor("test_db.desc")
print(desc.size_bytes, [f.name for f in desc])

with rdb.Storage("test_db", "test_db", storage_param="/path/to/dir") as st:
    print(len(st), st[0], st[-1])       # a negative index reads from the end
```

Values come back as `int`, `float`, `str`, `Fraction` for RATIONAL, a tuple for
the pair types, and `None` for NULL. A `Record` holds its own copy of the
payload, so it stays valid after the next read.

Every failure raises: missing files, bad directories, out-of-range indices, reads
from declared sources, and - since core phase 1 - a malformed `.desc` too
(`CorruptDescriptor`). Nothing reachable from this module ends the interpreter.

### Running a plan

```python
import retractordb as rdb

with rdb.Engine("/tmp/plan") as eng:            # storage dir for plans without STORAGE
    eng.compile("""
        DECLARE a INTEGER STREAM src, 1/2 FILE '/tmp/plan/data.txt'
        SELECT a*2 STREAM dst FROM src
    """)
    eng.run()                                   # every slot until the source runs dry
    print(eng.streams(), eng.slots_done, eng.time)
    print([row[0] for row in eng.rows("dst")])

    w = eng.window("dst", fields=["dst_0"], size=4, stride=2, dtype="float32")
    print(w.shape)                              # (n_windows, 4, 1)
    import torch
    t = torch.from_dlpack(w)                    # shares the window's memory, no torch dependency
```

`step()` advances one time slot and returns its index, or `None` once a declared
source has run dry (`compile(..., until_eof=False)` gives the daemon's wrapping
behaviour instead). `run(slots=None)` loops `step()` with the GIL released and
honours Ctrl-C. `rows()`, `record()`, `to_numpy()` and `window()` all return copies;
nothing aliases engine memory. Engine diagnostics go to
`logging.getLogger("retractordb")`, not to the cell's output.

`RQLSyntaxError` and `CompileError` (both `ConfigError`) carry the parser's and the
compiler's messages. `DUMP` and `SYSTEM` rule actions and `:ROTATION` are refused at
`compile()`: they need daemon state the embedded engine does not have
([`docs/core-phase-3.md`](../../docs/core-phase-3.md)).

`retractordb.torch` (`StreamDataset`, `as_tensor`) is the only module that imports
torch, and only when you import it. `DataLoader(..., num_workers=0)`: the engine
does not survive `fork`.

`notebooks/j1_engine.ipynb` walks through the same flow against a build tree.
