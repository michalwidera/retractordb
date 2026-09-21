"""RetractorDB for Python.

Two independent halves live under this one name:

``retractordb.client``
    The pure-Python client that talks to a running ``xretractor`` daemon through
    ``xqry``. Always importable.

``retractordb._core``
    The embedded engine - a compiled extension module that needs no daemon.
    Built only when the tree is configured with ``-DRDB_PYTHON=ON``; see
    ``docs/build-options.md``.

The embedded half is imported lazily, so a machine that never built the
extension still gets a working client. Reaching for an embedded name without the
extension raises ``ImportError`` naming the build flag, rather than failing
somewhere less obvious later on.
"""

from .client import Client, Subscription
from .models import Error, ReadTimeout, Field, Schema, Stream, Record

__all__ = [
    "Client",
    "Subscription",
    "Error",
    "ReadTimeout",
    "Field",
    "Schema",
    "Stream",
    "Record",
    # Zbudowane rozszerzenie (RDB_PYTHON=ON) - rozwiazywane leniwie w __getattr__.
    "ConfigError",
    "CorruptDescriptor",
    "Descriptor",
    "FieldType",
    "InternalError",
    "NoSuchStream",
    "RetractorDBError",
    "Storage",
    "StorageError",
    "load_descriptor",
]

# Nazwy dostarczane przez modul kompilowany. Trzymane osobno od __all__, zeby
# __getattr__ nie probowal importowac rozszerzenia dla nazwy, ktorej ono nie ma.
_CORE_NAMES = frozenset(
    {
        "ConfigError",
        "CorruptDescriptor",
        "Descriptor",
        "FieldType",
        "InternalError",
        "NoSuchStream",
        "RetractorDBError",
        "Storage",
        "StorageError",
        "load_descriptor",
    }
)

_BUILD_HINT = (
    "retractordb._core is not built in this installation. Configure the tree with "
    "-DRDB_PYTHON=ON and build it, or put the build's python/ directory on sys.path. "
    "See docs/build-options.md."
)


def __getattr__(name):
    """Resolve embedded-engine names on first use (PEP 562).

    Kolizja nazw jest zamierzona i rozstrzygnieta na korzysc klienta: ``Record``
    i ``Field`` istnieja po obu stronach. ``retractordb.Record`` to rekord
    klienta (models.py), a rekord silnika osadzonego bierze sie z
    ``retractordb._core.Record``. Nazwy z modeli sa zaimportowane wprost powyzej,
    wiec __getattr__ nigdy dla nich nie wstanie i nie moze ich przeslonic.
    """
    if name not in _CORE_NAMES:
        raise AttributeError(f"module {__name__!r} has no attribute {name!r}")

    try:
        from . import _core
    except ImportError as exc:  # pragma: no cover - zalezy od konfiguracji builda
        raise ImportError(_BUILD_HINT) from exc

    value = getattr(_core, name)
    globals()[name] = value  # kolejne odwolania ida juz zwykla sciezka
    return value


def __dir__():
    return sorted(__all__)
