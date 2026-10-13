"""J2: dense blocks, windows and the DLPack export.

Everything here is a copy of engine memory by design:
https://documentation.retractordb.com/appendices/python-jupyter-api.html#memory-ownership-numpy-and-dlpack.
The assertions that matter are the ones about shape, dtype and the fact that a
window stays what it was after the engine moves on.
"""

from __future__ import annotations

from pathlib import Path
import gc
import subprocess
import struct
import sys

import pytest

numpy = pytest.importorskip("numpy")

from test_engine import INPUT, doubling_plan, write_source  # noqa: E402


@pytest.fixture
def ran(rdb, tmp_path: Path):
    write_source(tmp_path)
    engine = rdb.Engine()
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
    with rdb.Engine() as engine:
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


@pytest.mark.parametrize("dtype", ["float32", "float64", "int32", "int64"])
@pytest.mark.parametrize("size,stride", [(1, 1), (3, 1), (3, 2), (8, 9), (9, 1)])
def test_windows_match_records_for_all_dtypes_and_strides(ran, dtype, size, stride) -> None:
    block = ran.to_numpy("dst", dtype=dtype)
    result = ran.window("dst", fields=["dst_0"], size=size, stride=stride).to_numpy()
    expected = numpy.array([block[start:start + size] for start in range(0, len(block) - size + 1, stride)],
                           dtype=dtype).reshape((-1, size, 1))
    numpy.testing.assert_array_equal(result, expected)
    assert result.flags.c_contiguous and result.flags.owndata


@pytest.mark.parametrize("dtype,value,expected", [
    ("int32", "2147483647", 2147483647), ("int32", "-2147483648", -2147483648),
    ("int32", "2147483647.75", 2147483647), ("int32", "-2147483648.75", -2147483648),
    ("int64", "9223372036854774784", 9223372036854774784),
    ("int64", "-9223372036854775808", -9223372036854775808),
])
def test_integer_export_accepts_representable_truncated_values(rdb, tmp_path, dtype, value, expected) -> None:
    source = write_source(tmp_path, [value])
    with rdb.Engine() as engine:
        engine.compile(f"DECLARE a DOUBLE STREAM src, 1 TEXTFILE '{source}'\nSELECT a STREAM dst FROM src")
        engine.run()
        assert engine.to_numpy("dst", dtype=dtype)[0, 0] == expected


@pytest.mark.parametrize("dtype,value", [
    ("int32", "2147483648"), ("int32", "-2147483649"), ("int32", "4294967295"),
    ("int64", "9223372036854775808"), ("int64", "-9223372036854777856"),
    ("int32", "inf"), ("int64", "-inf"),
])
def test_integer_export_refuses_overflow_and_infinity(rdb, tmp_path, dtype, value) -> None:
    source = tmp_path / "values.bin"
    source.write_bytes(struct.pack("<d", float(value)))
    with rdb.Engine() as engine:
        engine.compile(f"DECLARE a DOUBLE STREAM src, 1 BINFILE '{source}'\nSELECT a STREAM dst FROM src")
        engine.run()
        with pytest.raises(ValueError, match="outside integer dtype range"):
            engine.to_numpy("dst", dtype=dtype)
        assert not engine.failed
        assert engine.to_numpy("dst", dtype="float64").shape == (1, 1)


def test_uint_maximum_is_preserved_or_refused(rdb, tmp_path) -> None:
    source = write_source(tmp_path, [4294967295])
    with rdb.Engine() as engine:
        engine.compile(f"DECLARE a UINT STREAM src, 1 TEXTFILE '{source}'\nSELECT a STREAM dst FROM src")
        engine.run()
        assert engine.to_numpy("dst", dtype="int64")[0, 0] == 4294967295
        with pytest.raises(ValueError, match="outside integer dtype range"):
            engine.to_numpy("dst", dtype="int32")


def test_fields_arrays_nulls_and_empty_results(rdb, tmp_path) -> None:
    source = write_source(tmp_path, ["1 2 10", "3 4 abc", "5 6 30"])
    with rdb.Engine() as engine:
        engine.compile(f"DECLARE a INTEGER[2], b DOUBLE STREAM src, 1 TEXTFILE '{source}'\nSELECT AVG(a[0]:3) STREAM dst FROM src")
        engine.run(slots=2)
        names = [field.name for field in engine.schema("src") if field.name in ("a", "b")]
        assert names == ["a", "b"]
        block = engine.to_numpy("src", fields=names[::-1])
        numpy.testing.assert_array_equal(block, [[10, 1, 2], [numpy.nan, 3, 4], [30, 5, 6]])
        window = engine.window("src", fields=names[::-1], size=2).to_numpy()
        numpy.testing.assert_array_equal(window, numpy.stack([block[:2], block[1:]]))
        assert window.shape == (2, 2, 3)
        assert engine.to_numpy("src", first=3, count=0).shape == (0, 3)
        with pytest.raises(ValueError, match="null"):
            engine.to_numpy("src", fields=names[::-1], dtype="int64")
        with pytest.raises(ValueError):
            engine.window("src", fields=names, size=2, stride=0)


def test_tensors_and_blocks_survive_close_and_owner_destruction(rdb, tmp_path) -> None:
    torch = pytest.importorskip("torch")
    from retractordb.torch import as_tensor
    source = write_source(tmp_path)
    engine = rdb.Engine()
    engine.compile(doubling_plan(source))
    engine.run()
    block = engine.to_numpy("dst")
    window = engine.window("dst", fields=["dst_0"], size=3)
    tensor = as_tensor(window)
    expected = tensor.clone()
    assert tensor.data_ptr() == window.to_numpy().ctypes.data
    engine.close()
    del window, engine
    gc.collect()
    assert torch.equal(tensor, expected)
    assert block[:, 0].tolist() == [value * 2 for value in INPUT]


def test_torch_import_is_lazy_in_a_fresh_interpreter(module_root) -> None:
    result = subprocess.run([sys.executable, "-c", f"import sys; sys.path.insert(0, {str(module_root)!r}); "
                             "import retractordb; assert 'torch' not in sys.modules; "
                             "_=retractordb.Engine; assert 'torch' not in sys.modules"],
                            capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stderr


def test_dataset_refuses_workers_before_touching_the_engine(ran, monkeypatch) -> None:
    torch = pytest.importorskip("torch")
    from retractordb.torch import StreamDataset
    dataset = StreamDataset(ran, "dst", fields=["dst_0"], window=2)
    loader = torch.utils.data.DataLoader(dataset, num_workers=0, batch_size=None)
    assert len(list(loader)) == 7
    monkeypatch.setattr(torch.utils.data, "get_worker_info", lambda: object())
    ran.close()
    with pytest.raises(ValueError, match="num_workers=0"):
        list(dataset)
    with pytest.raises(ValueError, match="num_workers=0"):
        len(dataset)


def test_mps_float64_is_refused_before_device_access(ran) -> None:
    torch = pytest.importorskip("torch")
    from retractordb.torch import StreamDataset, as_tensor
    window = ran.window("dst", fields=["dst_0"], size=2, dtype="float64")
    with pytest.raises(ValueError, match="MPS.*float64.*float32"):
        as_tensor(window, device="mps")
    with pytest.raises(ValueError, match="MPS.*float64.*float32"):
        StreamDataset(ran, "dst", fields=["dst_0"], window=2, dtype=torch.float64, device=torch.device("mps"))
    assert as_tensor(window, device=torch.device("cpu")).dtype == torch.float64


def test_real_dataloader_worker_is_refused_in_a_subprocess(module_root, tmp_path) -> None:
    pytest.importorskip("torch")
    source = write_source(tmp_path)
    program = (f"import sys; sys.path.insert(0, {str(module_root)!r})\n"
               "import retractordb as rdb\nimport torch\nfrom retractordb.torch import StreamDataset\n"
               f"engine = rdb.Engine(); engine.compile({doubling_plan(source)!r}); engine.run()\n"
               "dataset = StreamDataset(engine, 'dst', fields=['dst_0'], window=2)\n"
               "engine.close()\n"
               "try:\n    for _ in torch.utils.data.DataLoader(dataset, num_workers=1, multiprocessing_context='fork'):\n        pass\n"
               "except ValueError as error:\n    assert 'num_workers=0' in str(error), error\n"
               "else:\n    raise AssertionError('worker accepted an Engine')\n")
    result = subprocess.run([sys.executable, "-c", program], capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
