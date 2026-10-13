"""PyTorch adapters: ``as_tensor`` and ``StreamDataset``.

This is the ONLY module in the package that imports torch, and nothing imports it
implicitly - ``import retractordb`` never touches torch. The reason is packaging
(docs/jupyter-integration.md section 4): a hard dependency on torch risks pip
reinstalling a 2 GB package against the wrong CUDA in Colab. The DLPack protocol
on ``Window`` makes the dependency unnecessary; this module is a convenience over
it, not a requirement.
"""

from __future__ import annotations

from collections.abc import Iterator, Sequence

try:
    import torch
    from torch.utils.data import IterableDataset
except ImportError as exc:  # pragma: no cover - zalezy od srodowiska
    raise ImportError(
        "retractordb.torch needs torch, which retractordb deliberately does not depend on; "
        "install it yourself (pip install torch) or use retractordb.Engine.window() with "
        "any DLPack consumer."
    ) from exc

from .engine import Engine, Window

# Nazwy dtype silnika -> torch. Blok z silnika powstaje juz w docelowym typie, wiec
# from_dlpack nie konwertuje niczego.
_ENGINE_DTYPE = {
    torch.float32: "float32",
    torch.float64: "float64",
    torch.int32: "int32",
    torch.int64: "int64",
}


def as_tensor(window: Window) -> torch.Tensor:
    """The window as a tensor sharing the window's memory (DLPack, no copy)."""
    return torch.from_dlpack(window)


class StreamDataset(IterableDataset):  # type: ignore[misc]
    """Windows of one stream as an ``IterableDataset``: each item is ``(size, n_values)``.

    Every ``__iter__`` re-reads the stream, so an engine that advanced between epochs
    contributes its new records to the next one. ``__len__`` is the window count at
    the moment it is asked.

    ``num_workers=0`` is structural, not a temporary limit: DataLoader workers are
    forked processes, and the engine holds open file descriptors and per-instance
    state that does not survive ``fork``. Open a separate ``Engine`` per worker in a
    ``worker_init_fn`` if you need parallel loading.
    """

    def __init__(
        self,
        engine: Engine,
        stream: str,
        *,
        fields: Sequence[str],
        window: int,
        stride: int = 1,
        dtype: torch.dtype = torch.float32,
    ) -> None:
        if dtype not in _ENGINE_DTYPE:
            raise ValueError(f"dtype must be one of {list(_ENGINE_DTYPE)}, not {dtype}")
        self.engine = engine
        self.stream = stream
        self.fields = tuple(fields)
        self.window = window
        self.stride = stride
        self.dtype = dtype

    def _windows(self) -> Window:
        return self.engine.window(
            self.stream, fields=self.fields, size=self.window, stride=self.stride, dtype=_ENGINE_DTYPE[self.dtype]
        )

    def __iter__(self) -> Iterator[torch.Tensor]:
        stacked = as_tensor(self._windows())
        for index in range(stacked.shape[0]):
            yield stacked[index]

    def __len__(self) -> int:
        return len(self._windows())

    def __repr__(self) -> str:
        return (
            f"StreamDataset({self.stream!r}, fields={list(self.fields)}, window={self.window}, "
            f"stride={self.stride}, dtype={self.dtype})"
        )


__all__: list[str] = ["StreamDataset", "as_tensor"]
