"""Re-entry: the shape a notebook puts the engine in, and the one it fails in.

A daemon constructs its state once. A notebook constructs it again on every cell
run, which is what makes the three process-wide globals visible:

* ``statusDesc``       - src/rdb/lib/DESCParser.cc:13
* ``fatalErrorRaised`` - src/include/fatalError.hpp:16
* the MEMORY maps      - src/rdb/lib/faccmemory.cc:13-16

Stage 1a does not remove them; phase 2 of the shared refactor does, and by now has:
``statusDesc`` is gone, ``fatalErrorRaised`` left the shared headers, and the MEMORY
maps became ``rdb::MemoryStore`` owned by ``rdb::embed::Engine``.

These tests remain the baseline that work must not regress. They are NOT the proof of
instance isolation, and it is worth being exact about why: isolation shows up on a
write, and the ``Storage`` binding is read-only by design. The proof lives in
``ut_embedEngine`` and, since J1 gave the binding a plan that writes, in
``test_engine.py::test_two_engines_do_not_share_a_volatile_stream``.
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
    """Overlapping instances, as far as a read-only binding can reach.

    What this DOES check: two live ``Storage`` objects over the same stream do not
    disturb each other's reads, in either order.

    What it CANNOT check, and did not when it was written: MEMORY-store isolation.
    That state only diverges on a *write*, and ``Storage`` is deliberately read-only
    (``module.cpp``). Phase 2 gave the MEMORY store an owner and
    ``rdb::embed::Engine`` to hold it, and ``ut_embedEngine`` asserts two engines do
    not share a stream. The Python half of that claim is
    ``test_engine.py::test_two_engines_do_not_share_a_volatile_stream``, written once
    J1 gave the binding a plan that writes.
    """
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as first:
        with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as second:
            assert tuple(first.read(0)) == (1, 10)
            assert tuple(second.read(1)) == (2, 20)
            # Kolejnosc odwrotna: odczyt z pierwszego PO odczycie z drugiego.
            # Gdyby ktorykolwiek stan byl dzielony, to jest miejsce, w ktorym widac.
            assert tuple(first.read(0)) == (1, 10)
