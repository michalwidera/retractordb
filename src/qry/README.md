# xqry

[RetractorDB](../../README.md) | [Manual (PL)](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xqry.html) | [Manual (EN)](https://documentation.retractordb.com/appendices/command-line-options/xqry.html)

`xqry` is the IPC client for a running [`xretractor`](../retractor/README.md). It lists instances, streams and plan variables, subscribes to live results, and submits ad-hoc statements or whole-plan replacements. Use [`xtrdb`](../rdb/README.md) for offline artifact files.

## Basic use

Discover instances, then inspect an existing stream named `temperature` on instance `demo`:

```bash
xqry --bus
xqry --server demo --dir
xqry --server demo --detail temperature
xqry --server demo --select temperature --elimitqry 10
```

Server-directed commands require a running engine. `--bus` reads accessible instance registries without contacting a server; `--help` also works without one. Prefer an explicit `--server` in scripts. Automatic routing and namespace scope are described in the manual.

## Output and plan control

- Raw text is the default subscription format; Graphite, InfluxDB and gnuplot output are also available.
- `--yaml` formats instance, stream, detail and variable listings. `--jsonl` supplies the versioned application protocol for `--hello`, `--dir`, `--detail` and `--select`.
- `--adhoc` adds one `SELECT`, `DECLARE` or `RULE` to an active plan. `--reset file.rql` submits a complete replacement plan, including directives. Reset ends existing subscriptions; clients must subscribe again.
- A reset accepted by a service is not a guarantee that its startup file was saved or that late activation will succeed. Follow the replacement contract and inspect the engine logs.

For example, a bounded JSON Lines subscription is:

```bash
xqry --server demo --jsonl --select temperature --elimitqry 10
```

Use `xqry --help` for available options. The [monitoring API](../../api/README.md) provides Python and C++ clients built on JSON Lines; the [embedded Python API](../../api/python/README.md) executes plans inside Python independently of `xqry`.

## Documentation

| Topic | PL | EN |
| --- | --- | --- |
| Commands, routing, output and timeouts | [xqry](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xqry.html) | [xqry](https://documentation.retractordb.com/appendices/command-line-options/xqry.html) |
| Ad-hoc statements | [Zapytania ad hoc](https://dokumentacja.retractordb.com/realizacja-zapytan/zapytania-ad-hoc.html) | [Ad-hoc queries](https://documentation.retractordb.com/query-execution/ad-hoc-queries.html) |
| Service configuration and plan replacement | [xretractor](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xretractor.html) | [xretractor](https://documentation.retractordb.com/appendices/command-line-options/xretractor.html) |
| JSON Lines and monitoring clients | [API monitorowania](https://dokumentacja.retractordb.com/zalaczniki/api-monitorowania-strumieni.html) | [Monitoring API](https://documentation.retractordb.com/appendices/stream-monitoring-api.html) |
