"""Type stubs for the compiled embedded-engine module.

Stage 1a gave the storage layer, read-only. J1 (core phase 3) added ``Engine``:
a compiled plan driven one time slot at a time. ``retractordb.engine.Engine``
subclasses it with the NumPy and window views; see docs/jupyter-integration.md.
"""

from enum import Enum
from fractions import Fraction
from os import PathLike
from types import TracebackType
from typing import Any, Sequence

class RetractorDBError(Exception):
    """Base of every error raised by the embedded engine."""

class NoSuchStream(RetractorDBError):
    """A descriptor file, storage directory or stream was not found."""

class StorageError(RetractorDBError):
    """The storage exists but cannot serve the request."""

class CorruptDescriptor(RetractorDBError):
    """A .desc file is empty or does not parse.

    The first error raised by the engine itself rather than by a guard in the
    binding: core phase 1, slice 1 turned ``loadDescriptorFile`` from a process
    exit into a throw.
    """

class ConfigError(RetractorDBError):
    """The storage cannot be configured as asked.

    An unknown ``storage_type``, a ``storage_param`` directory that is missing or
    is not a directory, an empty identifier, or a descriptor with no REF field and
    no storage directory. The input is wrong and the engine is intact, so this is
    the one worth catching and reporting to whoever typed it.
    """

class RQLSyntaxError(ConfigError):
    """The plan text does not parse. The message is the parser's, line and column included."""

class CompileError(ConfigError):
    """The plan parses but does not compile, or uses something the embedded engine lacks.

    The second kind is refused at ``compile()`` on purpose: DUMP rule actions reach
    the daemon's global model pointer, SYSTEM actions wait for a host callback that
    is off by default, and ROTATION reads the daemon's persistent counter.
    """

class IOError(RetractorDBError):  # noqa: A001 - shadows the builtin on purpose, see below
    """A file operation failed: open, read, append, overwrite, descriptor write.

    ``rdb::IOError`` on the C++ side. The name deliberately mirrors the C++ type and
    is reached as ``rdb.IOError``, never as a bare name, so it does not shadow the
    builtin in practice. Messages carry ``strerror(errno)`` rather than the return
    code, which for a failed ``open`` was always -1 and said nothing.
    """

class InternalError(RetractorDBError):
    """An engine invariant broke - a bug in RetractorDB, not in your input.

    ``rdb::LogicError`` on the C++ side. Raised where the engine used to assert by
    ending the process: a payload that was never attached, a record count that no
    longer matches the accessor. Catching this to carry on is a mistake; the state
    is already wrong. Report it.
    """

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
    def field_index(self, name: str) -> int:
        """Raises KeyError for an unknown field."""
    def byte_offset(self, name: str) -> int:
        """Raises KeyError for an unknown field."""
    def field_type_name(self, name: str) -> str:
        """Raises KeyError for an unknown field."""
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

    A missing file raises NoSuchStream; an empty or unparsable one raises
    CorruptDescriptor. Neither ends the process.
    """

class Engine:
    """One embedded engine: a compiled plan, driven one time slot at a time.

    The compiled half. ``retractordb.Engine`` (``retractordb.engine.Engine``) is
    the class to use: it adds ``rows()``, ``to_numpy()`` and ``window()``.
    """

    def __init__(self, storage_dir: str = "") -> None:
        """``storage_dir`` serves plans without a STORAGE directive; the directive wins."""
    def compile(self, rql: str, until_eof: bool = True) -> None:
        """Parse, compile and build the plan; raises RQLSyntaxError or CompileError.

        With ``until_eof`` (the default) declared sources are read once, without
        wrapping past the end of their file, and ``step()`` reports end of input.
        ``until_eof=False`` is the daemon's behaviour: the source wraps forever.
        """
    def step(self) -> int | None:
        """Advance one time slot; the slot index, or None at end of input. Releases the GIL."""
    def run(self, slots: int | None = None) -> int:
        """Advance up to ``slots`` slots (all, until end of input, when None).

        Returns the number of slots processed. Releases the GIL and honours
        KeyboardInterrupt, checked every 50 ms.
        """
    @property
    def has_plan(self) -> bool: ...
    @property
    def slots_done(self) -> int: ...
    @property
    def end_of_input(self) -> bool: ...
    @property
    def time(self) -> Fraction:
        """Plan time of the last slot, in seconds."""
    def streams(self) -> list[str]:
        """Stream ids in execution order, directives excluded."""
    def schema(self, stream: str) -> Descriptor:
        """Raises KeyError for a stream that is not in the plan."""
    def is_declared(self, stream: str) -> bool: ...
    def record_count(self, stream: str) -> int:
        """Records written since the plan started, retained or not."""
    def retained_from(self, stream: str) -> int:
        """Index of the oldest record still readable.

        0 for a stream on disk; higher for a declared source (its history ring) or a
        VOLATILE stream (a MEMORY ring the size the compiler asked for). Asking for an
        older record raises IndexError instead of returning someone else's record.
        """
    def record(self, stream: str, index: int) -> Record:
        """Oldest first; a negative index counts from the end. Always a copy."""
    def _block(self, stream: str, flat_fields: Sequence[int], first: int, count: int, dtype: str) -> Any:
        """Dense (count x len(flat_fields)) numpy block; null -> NaN. Used by to_numpy()."""
    def close(self) -> None:
        """Drop the plan and close its storages. Idempotent."""
    def __enter__(self) -> Engine: ...
    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        tb: TracebackType | None,
    ) -> bool: ...
