"""Type stubs for the compiled embedded-engine module.

Stage 1a: the storage layer, read-only. Everything above it - plans, RQL,
execution - arrives with phase J1; see docs/jupyter-integration.md.
"""

from enum import Enum
from fractions import Fraction
from os import PathLike
from types import TracebackType
from typing import Sequence

class RetractorDBError(Exception):
    """Base of every error raised by the embedded engine."""

class NoSuchStream(RetractorDBError):
    """A descriptor file, storage directory or stream was not found."""

class StorageError(RetractorDBError):
    """The storage exists but cannot serve the request."""

class FieldType(Enum):
    BYTE: int
    INTEGER: int
    UINT: int
    RATIONAL: int
    FLOAT: int
    DOUBLE: int
    INTPAIR: int
    IDXPAIR: int
    STRING: int
    NULLTYPE: int
    TYPE: int
    REF: int
    RETENTION: int
    RETMEMORY: int

class Field:
    @property
    def name(self) -> str: ...
    @property
    def length(self) -> int: ...
    @property
    def array_count(self) -> int: ...
    @property
    def type(self) -> FieldType: ...

class Descriptor(Sequence[Field]):
    @property
    def size_bytes(self) -> int:
        """Record width in bytes, configuration fields excluded."""
    @property
    def flat_element_count(self) -> int:
        """Number of value slots in a record. A STRING[N] field is one slot, not N."""
    def has_field(self, name: str) -> bool: ...
    def field_index(self, name: str) -> int: ...
    def byte_offset(self, name: str) -> int: ...
    def field_type_name(self, name: str) -> str: ...
    def storage_policy(self) -> tuple[str, int]: ...
    def __len__(self) -> int: ...
    def __getitem__(self, index: int) -> Field: ...  # type: ignore[override]

# Wartosc pola. RATIONAL wychodzi jako fractions.Fraction, a nie krotka - inaczej
# zlewalby sie z INTPAIR, ktory tez jest para liczb calkowitych. None to null.
Value = int | float | str | Fraction | tuple[int, int] | tuple[str, int] | None

class Record:
    """One record, holding its own copy of the payload.

    The copy is what makes a Record safe to keep: it does not alias the storage
    buffer, so it stays valid after the next read.
    """

    @property
    def index(self) -> int: ...
    @property
    def descriptor(self) -> Descriptor: ...
    def __len__(self) -> int: ...
    def __getitem__(self, index: int) -> Value: ...

class Storage:
    """Read access to one stream's records on disk.

    A negative index counts from the end, which is how C++ ``revRead()`` is
    spelled here.
    """

    def __init__(
        self,
        qry_id: str,
        file_name: str,
        storage_param: str = "",
        storage_type: str = "DEFAULT",
    ) -> None: ...
    @property
    def descriptor(self) -> Descriptor: ...
    @property
    def record_count(self) -> int: ...
    @property
    def is_declared(self) -> bool:
        """True for DEVICE and TEXTSOURCE sources, which cannot be read directly."""
    def read(self, index: int) -> Record: ...
    def __len__(self) -> int: ...
    def __getitem__(self, index: int) -> Record: ...
    def __enter__(self) -> Storage: ...
    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        tb: TracebackType | None,
    ) -> bool: ...

def load_descriptor(path: str | PathLike[str]) -> Descriptor:
    """Read a .desc file.

    A missing file raises NoSuchStream. A file that exists but holds an invalid
    descriptor still ends the process - that is FatalError, not an exception,
    and phase 1 of the shared refactor is what fixes it.
    """
