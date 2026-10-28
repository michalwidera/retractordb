# Python: Engine, Storage and Descriptor

[![Python >=3.10](https://img.shields.io/badge/Python-%3E%3D3.10-blue?logo=python)](https://documentation.retractordb.com/system-installation/python-jupyter.html)

[RetractorDB](../../README.md) | [API manual (PL)](https://dokumentacja.retractordb.com/zalaczniki/python-jupyter-api.html) | [API manual (EN)](https://documentation.retractordb.com/appendices/python-jupyter-api.html)

The embedded Python API executes RQL plans and reads stored results without a running service. It uses the compiled `retractordb._core` extension, with NumPy output and optional PyTorch integration for notebooks and applications.

## Interfaces

| Interface | Purpose | Main entry points |
| --- | --- | --- |
| `retractordb.Engine` | Compile and execute a plan on a logical timeline inside Python | `compile()`, `step()`, `run()`, `rows()`, `to_numpy()`, `window()` |
| `retractordb.Storage` | Read existing result storage offline | `read(i)`, `storage[i]`, `record_count`, `descriptor` |
| `retractordb.Descriptor` | Inspect a record schema: fields, types, offsets and record size | Obtain from `engine.schema(stream)`, `storage.descriptor` or `retractordb.load_descriptor(path)` |

`Engine` and `Storage` support context managers and `close()`. `Descriptor` is a sequence of fields, with `size_bytes` and `flat_element_count`; it does not open or execute a plan.

The separate `retractordb.client.Client` monitors a running engine through `xqry`. See the [monitoring API](../README.md) for Python and C++ clients.

## Installation and first example

The [`retractordb` package on PyPI](https://pypi.org/project/retractordb/) includes the embedded extension in compatible binary wheels. Follow the installation guide below for supported Python/platform combinations, an isolated environment and Jupyter or Colab setup. NumPy is required; PyTorch is optional. The standalone CLI/service installation is separate.

For a complete runnable example, including input data, plan execution, NumPy and DLPack, see [PL](https://dokumentacja.retractordb.com/zalaczniki/python-jupyter-api.html) / [EN](https://documentation.retractordb.com/appendices/python-jupyter-api.html).

Installing `pip install ./api/python` from the repository installs only the monitoring client and can replace a wheel containing `_core`. Use the root project's wheel build for embedding, or keep a client-only installation in a separate environment.

## Execution and storage contract

- `Engine` advances logical time without a wall-clock scheduler or IPC publication. Its streams are not visible to `xqry`.
- `compile()` closes the previous plan before compiling the new one; a failed compilation does not restore it. Use a slot budget for plans without an ending input.
- Engine inputs belong outside its owned output directory. Use a private temporary directory or an explicit directory satisfying the ownership rules in the manual.
- `Storage` is for offline result files. Opening it can rebuild metadata; it is not a live monitoring interface or a guarantee of filesystem-read-only access.
- Records, NumPy arrays and windows copy engine results. DLPack shares the window's CPU array, not the engine buffers.
- Serialize calls on each engine. For PyTorch integration use `DataLoader(num_workers=0)`. The manual specifies supported devices, errors and process-failure boundaries.

## Documentation and notebooks

| Topic | PL | EN |
| --- | --- | --- |
| Install wheels, Python, Jupyter and Colab | [Python i Jupyter](https://dokumentacja.retractordb.com/instalacja-systemu/python-jupyter.html) | [Python and Jupyter](https://documentation.retractordb.com/system-installation/python-jupyter.html) |
| API, types, ownership, errors and complete example | [API Python](https://dokumentacja.retractordb.com/zalaczniki/python-jupyter-api.html) | [Python API](https://documentation.retractordb.com/appendices/python-jupyter-api.html) |
| Build the extension and wheels | [Budowanie](https://dokumentacja.retractordb.com/instalacja-systemu/python-build.html) | [Source build](https://documentation.retractordb.com/system-installation/python-build.html) |
| Notebook and Colab validation | [Odbiór notebooków](https://dokumentacja.retractordb.com/zalaczniki/notebook-acceptance.html) | [Notebook acceptance](https://documentation.retractordb.com/appendices/notebook-acceptance.html) |

Local notebooks:

- [j0_storage.ipynb](notebooks/j0_storage.ipynb): offline storage and descriptors.
- [j1_engine.ipynb](notebooks/j1_engine.ipynb): execution, NumPy, DLPack and recompilation.
- [colab_quickstart.ipynb](notebooks/colab_quickstart.ipynb): wheel installation and optional CPU/CUDA training; follow its wheel and source-revision setup instructions.

See [type declarations](retractordb/_core.pyi) and [Python array/window helpers](retractordb/engine.py) for implementation details. Repository build and test rules are in [CLAUDE.md](../../CLAUDE.md).
