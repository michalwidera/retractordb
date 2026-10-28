# xretractor

[RetractorDB](../../README.md) | [Manual (PL)](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xretractor.html) | [Manual (EN)](https://documentation.retractordb.com/appendices/command-line-options/xretractor.html)

`xretractor` compiles RQL query sets and executes continuous stream-processing plans. It reads declared sources, evaluates transformations and rules, stores results, and publishes live streams for [`xqry`](../qry/README.md).

## Basic use

Given an RQL file `plan.rql`:

```bash
xretractor --onlycompile plan.rql
xretractor plan.rql --name demo --noanykey
```

The first command inspects the compiled plan without processing data. The second starts a named instance; stop it with Ctrl+C. To process finite file inputs without waiting for the wall clock:

```bash
xretractor plan.rql --name batch --no-clock --until-eof --noanykey
```

Use `xretractor --help` for runtime options and `xretractor --onlycompile --help` for plan inspection and diagram options. Installation and a complete first-plan example are linked below.

## Instances, configuration and services

- Instances can run concurrently with distinct names and disjoint stream/storage resources. Use `--name` or `--autoname` and discover them with `xqry --bus`.
- Starting without a query file creates an idle instance. `xqry --server demo --reset plan.rql` submits a whole replacement plan without restarting the process; existing subscriptions end when the plan changes.
- Configuration is optional TOML. `--config file.toml` selects one explicit file; otherwise the engine loads the platform's system and user layers. The manual defines keys, precedence and validation.
- Linux service setup uses systemd. For a service, an accepted reset also attempts to save its startup plan; `OK` does not guarantee durable persistence or successful late activation. Check service logs. Delivering a plan through a separate `xretractor plan.rql` invocation can instead restart a detected service; see the manual for targeting and permissions.

Clocked processing and optional Linux real-time scheduling do not provide hard real-time guarantees. For execution inside Python on a logical timeline, use the [embedded API](../../api/python/README.md).

## Documentation

| Topic | PL | EN |
| --- | --- | --- |
| CLI, configuration, services and plan replacement | [xretractor](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xretractor.html) | [xretractor](https://documentation.retractordb.com/appendices/command-line-options/xretractor.html) |
| Installation and source builds | [Instalacja](https://dokumentacja.retractordb.com/instalacja-systemu/) | [Installation](https://documentation.retractordb.com/system-installation/) |
| First plan and service use | [Uruchamianie](https://dokumentacja.retractordb.com/instalacja-systemu/running.html) | [Running](https://documentation.retractordb.com/system-installation/running.html) |
| Named instances and resource ownership | [Magistrala](https://dokumentacja.retractordb.com/architektura-systemu-przetwarzania-danych/wiele-instancji-i-magistrala.html) | [Instances and bus](https://documentation.retractordb.com/data-processing-system-architecture/multiple-instances-and-bus.html) |
| RQL syntax and operators | [Język zapytań](https://dokumentacja.retractordb.com/konstrukcja-jezyka-zapytan/) | [Query language](https://documentation.retractordb.com/query-language-construction/) |

See also [example plans](../../examples/) and the [declared-source state diagram](UML/storage-access-state.puml). Repository build and test rules are in [CLAUDE.md](../../CLAUDE.md).
