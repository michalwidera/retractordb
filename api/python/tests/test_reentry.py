"""Re-entry: the shape a notebook puts the engine in, and the one it fails in.

A daemon constructs its state once. A notebook constructs it again on every cell
run, which is what makes the three process-wide globals visible:

* ``statusDesc``       - src/rdb/lib/DESCParser.cc:13
* ``fatalErrorRaised`` - src/include/fatalError.hpp:16
* the MEMORY maps      - src/rdb/lib/faccmemory.cc:13-16

Stage 1a does not remove them; phase 2 of the shared refactor does. These tests
assert that the sequential case is clean today, so that the phase-2 work has a
baseline it must not regress, and so the first overlapping-instance failure is
caught here rather than in someone's notebook.
"""

from __future__ import annotations

from pathlib import Path


def test_open_and_close_one_hundred_times(rdb, plain_storage: Path) -> None:
    """Sequential construction must stay correct, not merely not crash."""
    for iteration in range(100):
        with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as storage:
            assert storage.record_count == 4, f"record count drifted on iteration {iteration}"
            assert tuple(storage.read(0)) == (1, 10), f"values drifted on iteration {iteration}"


def test_descriptor_parsing_is_repeatable(rdb, plain_storage: Path) -> None:
    """Targets statusDesc, which DESCParser sets to 'Fail' and never resets.

    Once phase 2 removes the global, this test keeps its meaning unchanged - it
    simply stops being the only thing standing between a stale parse state and a
    wrong answer.
    """
    path = str(plain_storage / "plain_file.desc")
    first = repr(rdb.load_descriptor(path))
    for _ in range(50):
        assert repr(rdb.load_descriptor(path)) == first


def test_two_storages_open_at_once(rdb, plain_storage: Path) -> None:
    """Overlapping instances - the case the globals will break first."""
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as first:
        with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as second:
            assert tuple(first.read(0)) == (1, 10)
            assert tuple(second.read(1)) == (2, 20)
            # Kolejnosc odwrotna: odczyt z pierwszego PO odczycie z drugiego.
            # Gdyby ktorykolwiek stan byl dzielony, to jest miejsce, w ktorym widac.
            assert tuple(first.read(0)) == (1, 10)
