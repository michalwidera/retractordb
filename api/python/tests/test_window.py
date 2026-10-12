"""J2: dense blocks, windows and the DLPack export.

Everything here is a copy of engine memory by design (docs/jupyter-integration.md
section 4), so the assertions that matter are the ones about shape, dtype and the
fact that a window stays what it was after the engine moves on.
"""

from __future__ import annotations

from pathlib import Path

import pytest

numpy = pytest.importorskip("numpy")

from test_engine import INPUT, doubling_plan, write_source  # noqa: E402


@pytest.fixture
def ran(rdb, tmp_path: Path):
    write_source(tmp_path)
    engine = rdb.Engine(str(tmp_path))
    engine.compile(doubling_plan(tmp_path / "data.txt"))
    engine.run()
    yield engine
    engine.close()


def test_to_numpy_gives_records_by_values(ran) -> None:
    block = ran.to_numpy("dst")
    assert block.shape == (len(INPUT), 1)
    assert block.dtype == numpy.float64
    assert block[:, 0].tolist() == [value * 2 for value in INPUT]

    narrow = ran.to_numpy("dst", fields=["dst_0"], dtype="int32", first=2, count=3)
    assert narrow.dtype == numpy.int32
    assert narrow[:, 0].tolist() == [60, 80, 100]


def test_to_numpy_rejects_bad_ranges_and_names(ran) -> None:
    with pytest.raises(IndexError):
        ran.to_numpy("dst", first=len(INPUT) + 1)
    with pytest.raises(IndexError):
        ran.to_numpy("dst", first=6, count=5)
    with pytest.raises(KeyError):
        ran.to_numpy("dst", fields=["nope"])
    with pytest.raises(ValueError):
        ran.to_numpy("dst", dtype="float16")
    with pytest.raises(KeyError):
        ran.to_numpy("typo")


def test_window_shape_follows_size_and_stride(ran) -> None:
    window = ran.window("dst", fields=["dst_0"], size=3, stride=2)
    # 8 rekordow, okno 3, krok 2: okna zaczynaja sie w 0, 2, 4 -> trzy okna.
    assert window.shape == (3, 3, 1)
    assert len(window) == 3
    assert window.dtype == numpy.float32
    assert window[0][:, 0].tolist() == [20.0, 40.0, 60.0]
    assert window[2][:, 0].tolist() == [100.0, 120.0, 140.0]
    assert window.fields == ("dst_0",)

    assert ran.window("dst", fields=["dst_0"], size=len(INPUT) + 1).shape == (0, len(INPUT) + 1, 1)
    with pytest.raises(ValueError):
        ran.window("dst", fields=["dst_0"], size=0)


def test_window_exports_dlpack_and_array_protocols(ran) -> None:
    window = ran.window("dst", fields=["dst_0"], size=4, stride=4, dtype="float64")
    assert window.__dlpack_device__() == (1, 0)  # kCPU
    shared = numpy.from_dlpack(window)
    assert shared.shape == (2, 4, 1)
    assert shared.tolist() == window.to_numpy().tolist()
    # from_dlpack dzieli pamiec z oknem - kopia z silnika jest dokladnie jedna.
    assert numpy.shares_memory(shared, window.to_numpy())

    as_array = numpy.asarray(window)
    assert as_array.shape == (2, 4, 1)
    assert numpy.asarray(window, dtype=numpy.float32).dtype == numpy.float32


def test_window_survives_the_engine_advancing(rdb, tmp_path: Path) -> None:
    write_source(tmp_path)
    with rdb.Engine(str(tmp_path)) as engine:
        engine.compile(doubling_plan(tmp_path / "data.txt"), until_eof=False)
        engine.run(slots=5)
        before = engine.window("dst", fields=["dst_0"], size=2)
        snapshot = before.to_numpy().copy()
        engine.run(slots=20)
        assert engine.window("dst", fields=["dst_0"], size=2).shape[0] > before.shape[0]
        assert before.to_numpy().tolist() == snapshot.tolist()


def test_block_over_a_declared_source_reads_its_retained_tail(ran) -> None:
    # Zrodlo trzyma tylko ogon historii o pojemnosci z kompilatora; to_numpy() zaczyna od
    # retained_from. Za koncem pliku zrodlo oddaje rekord all-null, czyli NaN.
    block = ran.to_numpy("src")
    assert block.shape == (ran.record_count("src") - ran.retained_from("src"), 1)
    assert all(numpy.isnan(value) or value in INPUT for value in block[:, 0])
    if ran.retained_from("src"):
        with pytest.raises(IndexError):
            ran.to_numpy("src", first=0)


def test_torch_round_trip_if_torch_is_installed(ran) -> None:
    torch = pytest.importorskip("torch")
    from retractordb.torch import StreamDataset, as_tensor

    window = ran.window("dst", fields=["dst_0"], size=3, stride=1)
    tensor = as_tensor(window)
    assert tuple(tensor.shape) == (6, 3, 1)
    assert tensor.dtype == torch.float32
    assert torch.equal(tensor, torch.from_dlpack(window))

    dataset = StreamDataset(ran, "dst", fields=["dst_0"], window=3, stride=1)
    items = list(dataset)
    assert len(items) == len(dataset) == 6
    assert items[0][:, 0].tolist() == [20.0, 40.0, 60.0]
