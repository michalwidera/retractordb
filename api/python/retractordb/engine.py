"""The embedded engine, as a notebook sees it: ``Engine`` and ``Window``.

``_core.Engine`` (nanobind) owns the plan and steps it; this module adds the
Python-side conveniences that have no business in C++: iteration over records,
NumPy blocks, and the windowed, DLPack-exporting view that a training loop wants
(docs/jupyter-integration.md section 4).

Everything here is a COPY of engine memory. That is the v1 decision from the
roadmap - "copy, do not pin" - and it is what makes a tensor safe to keep across
``step()``: the engine cannot overwrite what it no longer owns.

``numpy`` is imported lazily and is the only hard dependency of this module. Torch
is never imported here; ``retractordb.torch`` is the one place that does, and only
when the user asks for it.
"""

from __future__ import annotations

from collections.abc import Iterator, Sequence
from typing import Any

from . import _core

# Pola konfiguracyjne deskryptora nie zajmuja pozycji w rekordzie (descriptor.cc,
# isConfigurationField), wiec numeracja elementow splaszczonych je pomija.
_CONFIGURATION_TYPES = frozenset(
    {
        _core.FieldType.TYPE,
        _core.FieldType.REF,
        _core.FieldType.RETENTION,
        _core.FieldType.RETMEMORY,
    }
)

_DTYPES = ("float32", "float64", "int32", "int64")


def _flat_indices(descriptor: _core.Descriptor, fields: Sequence[str] | None) -> list[int]:
    """Flat element indices of ``fields`` (all data fields when None), in the given order.

    An array field ``x[3]`` contributes three consecutive slots; a STRING field is
    refused, because a dense numeric block has no place for it.
    """
    spans: dict[str, tuple[int, int, _core.FieldType]] = {}
    flat = 0
    for field in descriptor:
        if field.type in _CONFIGURATION_TYPES:
            continue
        width = 1 if field.type == _core.FieldType.STRING else field.array_count
        spans[field.name] = (flat, width, field.type)
        flat += width

    wanted = list(spans) if fields is None else list(fields)
    if not wanted:
        raise ValueError("fields must name at least one field")

    indices: list[int] = []
    for name in wanted:
        if name not in spans:
            raise KeyError(name)
        start, width, kind = spans[name]
        if kind in (_core.FieldType.STRING, _core.FieldType.INTPAIR, _core.FieldType.IDXPAIR):
            raise ValueError(f"field {name!r} is {kind.name}, which has no dense numeric form")
        indices.extend(range(start, start + width))
    return indices


class Window:
    """A stack of windows over one stream: shape ``(n_windows, size, n_values)``.

    Window ``w`` covers records ``w*stride`` to ``w*stride + size - 1``. The data is a
    contiguous NumPy array the window owns outright, so ``torch.from_dlpack(window)``
    shares it without copying again, and the engine advancing afterwards changes
    nothing here.
    """

    __slots__ = ("_array", "stream", "fields", "size", "stride")

    def __init__(self, array: Any, stream: str, fields: tuple[str, ...], size: int, stride: int) -> None:
        self._array = array
        self.stream = stream
        self.fields = fields
        self.size = size
        self.stride = stride

    @property
    def shape(self) -> tuple[int, int, int]:
        return tuple(self._array.shape)  # type: ignore[return-value]

    @property
    def dtype(self) -> Any:
        return self._array.dtype

    def __len__(self) -> int:
        return int(self._array.shape[0])

    def __getitem__(self, index: Any) -> Any:
        return self._array[index]

    def __array__(self, dtype: Any = None, copy: bool | None = None) -> Any:
        import numpy

        if dtype is None or numpy.dtype(dtype) == self._array.dtype:
            return self._array.copy() if copy else self._array
        if copy is False:
            raise ValueError("a dtype change needs a copy")
        return self._array.astype(dtype)

    # Protokol DLPack delegowany do numpy: sam eksport (kapsula, strides, deleter) jest
    # tam poprawny od numpy 1.22, a tensor zbudowany przez torch.from_dlpack dzieli
    # pamiec z ta tablica - kopia z silnika jest dokladnie jedna.
    def __dlpack__(self, *args: Any, **kwargs: Any) -> Any:
        return self._array.__dlpack__(*args, **kwargs)

    def __dlpack_device__(self) -> tuple[int, int]:
        return self._array.__dlpack_device__()

    def to_numpy(self) -> Any:
        """The underlying array itself (no copy)."""
        return self._array

    def __repr__(self) -> str:
        return (
            f"<Window {self.stream} fields={list(self.fields)} shape={self.shape} "
            f"stride={self.stride} dtype={self._array.dtype}>"
        )


class Engine(_core.Engine):
    """One embedded engine, with the record and array views a notebook wants.

    Construct, ``compile(rql)``, then ``step()`` or ``run()``; read results with
    ``rows()``, ``to_numpy()`` or ``window()``. Use it as a context manager to close
    the plan's storages deterministically.
    """

    def rows(self, stream: str, *, first: int | None = None, count: int | None = None) -> Iterator[_core.Record]:
        """Records of ``stream`` from ``first``, oldest first. Each is an independent copy.

        ``first`` defaults to the oldest record still readable (``retained_from``): a
        stream on disk keeps everything, a declared source or a VOLATILE stream only
        a tail the size the compiler asked for.
        """
        total = self.record_count(stream)
        if first is None:
            first = self.retained_from(stream)
        stop = total if count is None else min(total, first + count)
        for index in range(first, stop):
            yield self.record(stream, index)

    def to_numpy(
        self,
        stream: str,
        *,
        fields: Sequence[str] | None = None,
        dtype: str = "float64",
        first: int | None = None,
        count: int | None = None,
    ) -> Any:
        """A dense ``(records, values)`` array of ``fields`` (all data fields by default).

        ``first`` defaults to ``retained_from(stream)``, as in ``rows()``. Nulls become
        NaN, which is why the default dtype is a float; an integer dtype raises
        ValueError on the first null rather than writing a zero.
        """
        if dtype not in _DTYPES:
            raise ValueError(f"dtype must be one of {_DTYPES}, not {dtype!r}")
        total = self.record_count(stream)
        oldest = self.retained_from(stream)
        if first is None:
            first = oldest
        if first < oldest or first > total:
            raise IndexError(f"first={first} is outside records {oldest}..{total} of {stream!r}")
        if count is None:
            count = total - first
        if count < 0 or first + count > total:
            raise IndexError(f"count={count} reaches past the {total} records of {stream!r}")
        indices = _flat_indices(self.schema(stream), fields)
        return self._block(stream, indices, first, count, dtype)

    def window(
        self,
        stream: str,
        *,
        fields: Sequence[str],
        size: int,
        stride: int = 1,
        dtype: str = "float32",
    ) -> Window:
        """Windows of ``size`` records, ``stride`` apart, over ``fields`` of ``stream``.

        Shape ``(n_windows, size, n_values)``, where ``n_values`` counts array
        elements, not field names. Fewer than ``size`` records gives zero windows,
        not an error - a training loop polls for that.
        """
        if size <= 0:
            raise ValueError("size must be positive")
        if stride <= 0:
            raise ValueError("stride must be positive")
        import numpy

        block = self.to_numpy(stream, fields=fields, dtype=dtype)
        records, values = block.shape
        n_windows = 0 if records < size else (records - size) // stride + 1
        stacked = numpy.empty((n_windows, size, values), dtype=block.dtype)
        for w in range(n_windows):
            start = w * stride
            stacked[w] = block[start : start + size]
        return Window(stacked, stream, tuple(fields), size, stride)

    def __enter__(self) -> Engine:
        return self

    def __repr__(self) -> str:
        return _core.Engine.__repr__(self)
