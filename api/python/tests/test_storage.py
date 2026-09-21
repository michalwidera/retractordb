"""Reading records through the embedded storage binding."""

from __future__ import annotations

from pathlib import Path

import pytest


def open_plain(rdb, directory: Path):
    return rdb.Storage("plain_file", "plain_file", storage_param=str(directory))


def test_storage_reports_its_shape(rdb, plain_storage: Path) -> None:
    with open_plain(rdb, plain_storage) as storage:
        assert storage.record_count == 4
        assert len(storage) == 4
        assert not storage.is_declared
        assert [field.name for field in storage.descriptor] == ["a", "b"]


def test_reads_the_recorded_values(rdb, plain_storage: Path) -> None:
    """Values come from generate_storage_map_fixtures.py: (1,10) (2,20) (3,30) (4,40)."""
    with open_plain(rdb, plain_storage) as storage:
        first = storage.read(0)
        assert len(first) == 2
        assert (first[0], first[1]) == (1, 10)
        assert first.index == 0

        assert tuple(storage.read(1)) == (2, 20)


def test_null_metadata_reaches_python_as_none(rdb, plain_storage: Path) -> None:
    """The meta index marks record 2 partially null and record 3 entirely null.

    Source: the segment layout written by generate_storage_map_fixtures.py and
    rendered in out-plain.local.txt as [====]2 [----]1 [~~~~]1 [XXXX]gap.
    """
    with open_plain(rdb, plain_storage) as storage:
        assert storage.read(2)[0] is None
        assert tuple(storage.read(3)) == (None, None)


def test_negative_index_reads_from_the_end(rdb, plain_storage: Path) -> None:
    """A negative index is how revRead() is spelled in Python."""
    with open_plain(rdb, plain_storage) as storage:
        assert tuple(storage[-4]) == tuple(storage[0])
        assert storage[-1].index == 3


def test_out_of_range_raises_rather_than_returning_zeros(rdb, plain_storage: Path) -> None:
    """storage::read() logs and hands back a zeroed record for a bad index.

    A silently wrong value is worse than an exception, so the binding checks the
    range itself. This test pins that guard - without it the call below would
    quietly succeed and return zeros.
    """
    with open_plain(rdb, plain_storage) as storage:
        with pytest.raises(IndexError):
            storage.read(4)
        with pytest.raises(IndexError):
            storage.read(-5)


def test_a_record_survives_the_next_read(rdb, plain_storage: Path) -> None:
    """Record holds its own payload copy, so it does not alias the storage buffer."""
    with open_plain(rdb, plain_storage) as storage:
        kept = storage.read(0)
        storage.read(1)
        assert tuple(kept) == (1, 10)


def test_a_shadow_update_wins_over_the_data_file(rdb, shadowed_storage: Path) -> None:
    """The read path consults the shadow first, and that is visible from Python.

    posixBinaryFileWithShadow::read() calls shadowFind() before touching the data
    file (faccposixshd.cc:221-232), and this fixture's shadow carries one entry
    for byte offset 0 holding (0,0). So record 0 reads as the pending update
    while records 1-3 fall through to the data file.

    Pinned here because it is not obvious from the API and because it is the
    reason `plain_storage` omits the shadow - see the note in conftest.py.
    """
    with open_plain(rdb, shadowed_storage) as storage:
        assert tuple(storage.read(0)) == (0, 0)
        assert tuple(storage.read(1)) == (2, 20)


def test_missing_storage_directory_raises(rdb, tmp_path: Path) -> None:
    with pytest.raises(rdb.NoSuchStream):
        rdb.Storage("plain_file", "plain_file", storage_param=str(tmp_path / "absent"))


def test_missing_descriptor_raises(rdb, tmp_path: Path) -> None:
    with pytest.raises(rdb.NoSuchStream):
        rdb.Storage("no_such_stream", "no_such_stream", storage_param=str(tmp_path))


def test_empty_identifiers_are_rejected(rdb, plain_storage: Path) -> None:
    """StoragePaths ends the process on an empty id; the binding refuses first."""
    with pytest.raises(ValueError):
        rdb.Storage("", "plain_file", storage_param=str(plain_storage))
    with pytest.raises(ValueError):
        rdb.Storage("plain_file", "", storage_param=str(plain_storage))


def test_errors_share_one_base(rdb) -> None:
    assert issubclass(rdb.NoSuchStream, rdb.RetractorDBError)
    assert issubclass(rdb.StorageError, rdb.RetractorDBError)
