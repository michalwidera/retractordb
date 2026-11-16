---
name: python-binding-check
description: Przepis na lokalną kontrolę wiązania Pythona (#447) - venv, trasa RDB_PYTHON=ON w build/Debug (ctest -L python) albo scripts/check-python.sh, tryb jawny conftest.py, co zapisać z wynikiem. Wywołaj, gdy diff dotyka źródeł silnika pod src/, konfiguracji CMake lub Conan używanej przez _core, pakietu albo testów Pythona.
---

# Python binding - przepis kontroli

Reguła (kiedy kontrola jest obowiązkowa) jest w `CLAUDE.md`, sekcja *Session end*: wiersz tabeli i akapit *Python binding*. Ten plik zawiera przepis.

Prepare Debug Conan dependencies and a Python 3.10+ venv with nanobind, NumPy, pytest and torch. `scripts/python-venv.sh` prepares the first three; install torch separately in that venv (CPU is sufficient, no GPU is required).

**Preferred route: `RDB_PYTHON=ON` in `build/Debug`.** Configure it once with `cmake -DRDB_PYTHON=ON -DPython_EXECUTABLE=<repo>/.venv-python/bin/python3 .` (a reconfigure deletes `build/Debug/test`; run `ninja` before `ctest`). Every `ninja` then builds `_core` next to the engine, and `ninja test` runs the binding as two ctest entries labelled `python`: `python_stage` refreshes the package's `.py`/`.pyi` copies next to `_core` (its `POST_BUILD` copy runs only when `_core` relinks), and `python_binding` runs `pytest api/python/tests`. That costs seconds and no second compilation of the engine. `ninja test-valgrind` does not include them. A build without `RDB_PYTHON=ON` registers neither entry.

**Fallback: `scripts/check-python.sh`** for a `build/Debug` that keeps `RDB_PYTHON=OFF`:

```bash
scripts/check-python.sh --python .venv-python/bin/python3 --jobs 4
```

The script keeps the separate `build/Check-Python` directory with `RDB_PYTHON=ON`, configures it only on the first run or when the interpreter changes (ninja reconfigures it after CMake changes), rebuilds `_core`, reference `xretractor` and `xqry` and the core no-exceptions compile checks, and runs `ctest -L python` there. Measured on 2026-10-21: a first run from an empty directory takes about 3 min (4 jobs, no ccache), a run with nothing to rebuild about 6 s. It does not install project binaries.

On both routes the ctest entry sets `RDB_PYTHON_MODULE_DIR` and `RDB_XRETRACTOR`, which put `api/python/tests/conftest.py` into explicit mode: missing NumPy or torch, a `_core` from another directory, a stale package copy, or an `xretractor`/`xqry` in `PATH` other than the build's targets fails the run instead of skipping tests. Skipped tests are unperformed coverage and must be reported with their reasons. Record the source revision, build directory, interpreter, pytest summary and elapsed time; `ctest -N -V -R python_binding` shows the interpreter, `_core` directory and reference paths, and JUnit results are stored in `<build>/python-test-results.xml`.
