# RetractorDB

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![CircleCI](https://dl.circleci.com/status-badge/img/circleci/JNsMwQGBP38ArHVd2CGwAF/WU9ujc7GeVXvtLjU2b5mSf/tree/master.svg?style=shield)](https://dl.circleci.com/status-badge/redirect/circleci/JNsMwQGBP38ArHVd2CGwAF/WU9ujc7GeVXvtLjU2b5mSf/tree/master)
[![GitHub release](https://img.shields.io/github/v/release/michalwidera/retractordb)](https://github.com/michalwidera/retractordb/releases/latest)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue?logo=cplusplus)](https://documentation.retractordb.com/system-installation/source-build.html)
[![PyPI version](https://img.shields.io/pypi/v/retractordb)](https://pypi.org/project/retractordb/)
[![Documentation PL](https://img.shields.io/badge/docs-PL-blue)](https://dokumentacja.retractordb.com/)
[![Documentation EN](https://img.shields.io/badge/docs-EN-blue)](https://documentation.retractordb.com/)

**RetractorDB** is an Edge Signal Processing Engine for processing and recording regular time series close to their sources. It combines a declarative query language (RQL), continuous signal processing and binary storage for telemetry, monitoring and IoT applications.

> ⚠️ **This is work in progress:** Active development is ongoing.

[Website](https://retractordb.com/) | [Documentation (PL)](https://dokumentacja.retractordb.com/) | [Documentation (EN)](https://documentation.retractordb.com/) | [Releases](https://github.com/michalwidera/retractordb/releases)

## Overview

- **Regular streams:** exact rational sampling intervals, synchronization, interleaving, rate conversion and sliding windows.
- **Continuous queries:** RQL compiles stream transformations, field expressions and rules into an execution plan.
- **Execution and storage:** sequential plan evaluation, named engine instances, shared-memory access to live results, and binary artifacts with descriptors and metadata.
- **Python and Jupyter:** an embedded engine with NumPy output, DLPack windows and optional PyTorch integration.

Linux x86-64 and ARM64 are the deployment platforms. macOS on Apple silicon is a development and test platform built from source; see the build guide for requirements and tested configurations. The standalone engine supports clocked processing and optional Linux real-time scheduling, without hard real-time guarantees. The embedded Python engine runs on a logical timeline.

## Components

| Component | Role |
| --- | --- |
| [`xretractor`](src/retractor/README.md) | Compiles and executes RQL plans, standalone or as a service. |
| [`xqry`](src/qry/README.md) | Reads live results, inspects instances and sends ad-hoc queries or plan replacements through IPC. |
| [`xtrdb`](src/rdb/README.md) | Inspects and edits offline binary storage, descriptors and metadata. |
| [Python `Engine`, `Storage`, `Descriptor`](api/python/README.md) | Executes plans inside Python and accesses offline storage without a running service. |

The [monitoring clients](api/README.md) for Python and C++ use `xqry` to access a running engine. They are separate from embedded plan execution.

## Getting started

The Polish manual is the canonical documentation; the English manual is its synchronized translation. Installation, configuration and runnable examples are maintained there:

| Task | PL | EN |
| --- | --- | --- |
| Install Linux binaries: portable archives, DEB, systemd, upgrades | [Instalacja](https://dokumentacja.retractordb.com/instalacja-systemu/linux-releases.html) | [Linux releases](https://documentation.retractordb.com/system-installation/linux-releases.html) |
| Build from source: Linux and macOS, Conan 2, CMake, Ninja | [Budowanie](https://dokumentacja.retractordb.com/instalacja-systemu/source-build.html) | [Source build](https://documentation.retractordb.com/system-installation/source-build.html) |
| Run a first plan or load a service plan | [Uruchamianie](https://dokumentacja.retractordb.com/instalacja-systemu/running.html) | [Running](https://documentation.retractordb.com/system-installation/running.html) |
| Set up Python, Jupyter or Colab | [Python i Jupyter](https://dokumentacja.retractordb.com/instalacja-systemu/python-jupyter.html) | [Python and Jupyter](https://documentation.retractordb.com/system-installation/python-jupyter.html) |
| Use the embedded Python API, NumPy and PyTorch | [API Python](https://dokumentacja.retractordb.com/zalaczniki/python-jupyter-api.html) | [Python API](https://documentation.retractordb.com/appendices/python-jupyter-api.html) |
| Configure the engine, storage and service | [xretractor](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xretractor.html) | [xretractor](https://documentation.retractordb.com/appendices/command-line-options/xretractor.html) |
| Learn RQL | [Język zapytań](https://dokumentacja.retractordb.com/konstrukcja-jezyka-zapytan/) | [Query language](https://documentation.retractordb.com/query-language-construction/) |
| Understand the stream algebra | [Podstawy matematyczne](https://dokumentacja.retractordb.com/podstawy-matematyczne/) | [Mathematical foundations](https://documentation.retractordb.com/mathematical-foundations/) |

For local examples, see [RQL plans](examples/) and [Python notebooks](api/python/notebooks/). The manuals also include signal-filter, ECG and OHLC walkthroughs: [PL](https://dokumentacja.retractordb.com/przyklady-zastosowan/) / [EN](https://documentation.retractordb.com/usage-examples/).

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines and [CLAUDE.md](CLAUDE.md) for repository build, test and collaboration rules. Release packaging is documented in [scripts/release_package/README.md](scripts/release_package/README.md).

## License and author

[MIT License](LICENSE). Created and maintained by **Michal Widera** (2003-2026).

Contact: michal@widera.com.pl
