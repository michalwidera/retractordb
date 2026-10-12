"""Shared fixtures for the embedded-engine tests.

The suite runs against a build tree, not an installation: CMake writes the
compiled module into ``<build>/python/retractordb/`` and records that directory
in the cache as ``RDB_PYTHON_MODULE_DIR``. Nothing has to be pip-installed.
"""

from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[3]

# Magazyn testowy wytworzony przez silnik i wersjonowany w drzewie: 4 rekordy
# (BYTE a, BYTE b) o wartosciach (1,10) (2,20) (3,30) (4,40), z indeksem meta
# opisujacym nulle i przerwe. Zrodlo: generate_storage_map_fixtures.py w tym
# samym katalogu; ksztalt potwierdza out-plain.local.txt.
FIXTURE_DIR = REPO_ROOT / "test" / "IntegrationTest" / "issue153_storagemap_meta_cases"
#
# plain_file.shadow jest CELOWO pominiety. Magazyn typu DEFAULT to
# groupFile<posixBinaryFileWithShadow> (makeAccessor, accessorFactory.cc), a jego read()
# zaglada najpierw do cienia i dopiero potem do pliku danych
# (faccposixshd.cc). Cien w tym zestawie niesie jeden wpis dla pozycji 0
# o wartosci (0,0), wiec skopiowany razem z reszta przeslanialby rekord 0
# zerami. Test SHADOW_FIXTURE_FILES nizej pokazuje to wprost.
FIXTURE_FILES = (
    "plain_file",
    "plain_file.desc",
    "plain_file.meta",
)

SHADOW_FIXTURE_FILES = FIXTURE_FILES + ("plain_file.shadow",)


def _module_root() -> Path | None:
    """Find the importable root holding a built ``retractordb._core``."""
    declared = os.environ.get("RDB_PYTHON_MODULE_DIR")
    if declared:
        return Path(declared)

    # Kolejnosc jest ustalona, a nie przypadkowa: build/Debug bywa swiezszy przy
    # pracy nad kodem, ale przy dwoch konfiguracjach chcemy powtarzalnego wyboru,
    # wiec bierzemy pierwszy pasujacy z posortowanej listy i nie zgadujemy dalej.
    for candidate in sorted((REPO_ROOT / "build").glob("*/python")):
        if any((candidate / "retractordb").glob("_core*")):
            return candidate
    return None


def pytest_configure(config: pytest.Config) -> None:
    root = _module_root()
    if root is not None:
        sys.path.insert(0, str(root))
    if os.environ.get("RDB_PYTHON_MODULE_DIR"):
        try:
            import retractordb._core as core
            import numpy  # noqa: F401
            if os.environ.get("RDB_XRETRACTOR"):
                import torch  # noqa: F401
        except ImportError as exc:
            raise pytest.UsageError(f"explicit embedded acceptance environment is incomplete: {exc}") from exc
        if Path(core.__file__).resolve().parent != (root / "retractordb").resolve():
            raise pytest.UsageError(f"wrong _core provenance: {core.__file__}; expected {root}")


@pytest.fixture(scope="session")
def xretractor(module_root: Path) -> Path:
    executable = Path(os.environ.get("RDB_XRETRACTOR", module_root.parent / "src/retractor/xretractor")).resolve()
    if not executable.is_file():
        if os.environ.get("RDB_XRETRACTOR"):
            pytest.fail(f"explicit acceptance requires a built xretractor: {executable}")
        pytest.skip("reference tests require a built xretractor or RDB_XRETRACTOR")
    return executable


@pytest.fixture(scope="session")
def module_root() -> Path:
    """The importable root, for tests that must start a fresh interpreter."""
    root = _module_root()
    if root is None:
        pytest.skip("retractordb._core is not built")
    return root


@pytest.fixture(scope="session")
def rdb():
    """The embedded module, or a skip explaining exactly how to get one."""
    try:
        import retractordb as module
        module.Storage  # wymusza leniwy import _core  # noqa: B018
    except (ImportError, AttributeError) as exc:
        pytest.skip(
            f"retractordb._core is not built ({exc}). Configure with -DRDB_PYTHON=ON, "
            "build, then re-run; or set RDB_PYTHON_MODULE_DIR to the build's python/ directory."
        )
    return module


@pytest.fixture
def plain_storage(tmp_path: Path) -> Path:
    """A private copy of the plain_file fixture.

    Copied rather than opened in place on purpose: opening a storage can write to
    its ``.meta`` sidecar (startup gap detection), and a test must not dirty a
    versioned fixture.
    """
    for name in FIXTURE_FILES:
        shutil.copy2(FIXTURE_DIR / name, tmp_path / name)
    return tmp_path


@pytest.fixture
def shadowed_storage(tmp_path: Path) -> Path:
    """The same fixture, with its shadow file - one pending update for record 0."""
    for name in SHADOW_FIXTURE_FILES:
        shutil.copy2(FIXTURE_DIR / name, tmp_path / name)
    return tmp_path
