# Python API

The distribution contains two independent interfaces:

- `retractordb.client`: pure-Python monitoring through `xqry`, requiring a running server.
- `retractordb.Engine`, `Storage`, `Descriptor`: the optional compiled embedded API, running inside Python without a daemon or bus publication.

The embedded engine computes on a logical timeline and provides no real-time guarantees. NumPy arrays and windows copy engine results; DLPack shares the window array. PyTorch is optional and imported only when explicitly requested. Serialize calls on each engine and use `DataLoader(num_workers=0)`.

Canonical documentation is maintained in Polish and synchronized in English:

| Topic | Polish | English |
| --- | --- | --- |
| Installation | [Python and Jupyter](https://dokumentacja.retractordb.com/instalacja-systemu/python-jupyter.html) | [Python and Jupyter](https://documentation.retractordb.com/system-installation/python-jupyter.html) |
| API, errors, storage ownership, complete example | [API Python](https://dokumentacja.retractordb.com/zalaczniki/python-jupyter-api.html) | [Python API](https://documentation.retractordb.com/appendices/python-jupyter-api.html) |
| Source builds and wheels | [Python build](https://dokumentacja.retractordb.com/instalacja-systemu/python-build.html) | [Python build](https://documentation.retractordb.com/system-installation/python-build.html) |
| Colab acceptance and notebook smoke | [Protocol](https://dokumentacja.retractordb.com/zalaczniki/notebook-acceptance.html) | [Protocol](https://documentation.retractordb.com/appendices/notebook-acceptance.html) |
| Monitoring client | [Monitoring API](https://dokumentacja.retractordb.com/zalaczniki/api-monitorowania-strumieni.html) | [Monitoring API](https://documentation.retractordb.com/appendices/stream-monitoring-api.html) |

Use an explicitly supplied compatible wheel file or direct HTTPS URL for the embedded library. This guide does not assume PyPI publication. `pip install ./api/python` installs the client only and replaces an existing wheel of the same distribution name, including `_core`; keep separate environments.

Handled core failures become `RetractorDBError` subclasses. `InternalError` taints the module until interpreter restart; an environment failure can stop the plan until recompilation. Failures in `noexcept` or destructors can still terminate the process. Standard name, index and conversion errors can use normal Python exceptions. See the API pages for the complete contract.

Notebooks in this directory:

- [j0_storage.ipynb](notebooks/j0_storage.ipynb): offline descriptors and storage.
- [j1_engine.ipynb](notebooks/j1_engine.ipynb): plan execution, NumPy, DLPack and recompilation.
- [colab_quickstart.ipynb](notebooks/colab_quickstart.ipynb): wheel installation and optional CPU/CUDA training.

The binding links `rdbembed` and `retractorcore`, never the service library. Implementation history lives in the [integration history](https://documentation.retractordb.com/appendices/embedded-integration-history.html). Build/test and commit policies remain in the repository's `CLAUDE.md`.
