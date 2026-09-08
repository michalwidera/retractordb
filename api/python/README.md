# Python API

Requires Linux, Python 3.10+, and matching `xqry` / `xretractor` binaries.
There are no runtime dependencies outside the Python standard library.

```sh
python3 -m venv .venv-api
.venv-api/bin/python -m pip install ./api/python
.venv-api/bin/python api/python/examples/monitor.py laboratory temperature --limit 10
```

The server must already be running, for example
`xretractor query.rql --name laboratory -k` in another terminal. Create the
query's storage directory first when using `STORAGE`.

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

`Client(server, *, xqry="xqry", timeout=5.0)` uses seconds for timeouts.
`subscribe(stream, *, limit=0, idle_timeout=0.0, capacity=1024)` starts an
independent child. A zero limit means unlimited records. `samples.schema`
is available when `subscribe()` returns. `samples.pid`, `samples.returncode`
and `samples.closed` expose process/lifecycle state.

`samples.next(timeout=0.5)` returns a `Record`, returns `None` at normal end,
or raises `ReadTimeout` while keeping the subscription open. Iteration uses
an unlimited read wait. `Error.code` identifies other failures. Use `close()`
or leave the context to stop. Breaking a loop alone does not close its iterator;
keep the surrounding `with`.

`Record.values` maps field names to scalars or lists for arrays with more than
one element. NULL is `None`, including inside arrays. Rationals and stream
periods use exact `Fraction` objects. `Schema.fields` retains field order.

```python
with Client("laboratory") as db:
    with db.subscribe("temperature") as a, db.subscribe("pressure") as b:
        print(a.next(timeout=2.0).values)
        print(b.next(timeout=2.0).values)
```

For a GUI, use timed reads from its worker or timer and handle `ReadTimeout`.
Serialize calls on each handle. The package does not impose an event loop or
invoke callbacks on the GUI thread. The example handles Ctrl+C; applications
using other termination signals must arrange context cleanup themselves.

See [the shared contract](../README.md) for delivery and lifecycle limits.
