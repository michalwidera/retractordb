"""Re-entry: the shape a notebook puts the engine in, and the one it fails in.

A daemon constructs its state once. A notebook constructs it again on every cell
run, which is what makes process-wide state visible:

* ``fatalErrorRaised`` - src/include/fatalError.hpp (raised only outside the
  throw-mode scope, so the binding never sets it; it describes the xretractor
  exit path and stays process-wide by decision (a) of #463)
* the MEMORY map       - now ``rdb::MemoryStore``, owned by ``rdb::embed::Engine``
  (phase 2, #446 C2); storages built without an engine keep the process default

These tests remain the baseline that work must not regress. They are NOT the proof
of instance isolation, and it is worth being exact about why: isolation shows up on
a write, and the ``Storage`` binding is read-only by design. The proof lives in
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
    """A parse state that outlived one read would change the next one.

    The DESC parser reports its status per call; this test pins that repeated reads
    of one file give one answer.
    """
    path = str(plain_storage / "plain_file.desc")
    first = repr(rdb.load_descriptor(path))
    for _ in range(50):
        assert repr(rdb.load_descriptor(path)) == first


def test_two_storages_open_at_once(rdb, plain_storage: Path) -> None:
    """Overlapping instances, as far as a read-only binding can reach.

    What this DOES check: two live ``Storage`` objects over the same stream do not
    disturb each other's reads, in either order.

    What it CANNOT check: MEMORY-store isolation. That state only diverges on a
    *write*, and ``Storage`` is deliberately read-only. Phase 2 gave the MEMORY
    store an owner and ``rdb::embed::Engine`` to hold it, and ``ut_embedEngine``
    asserts two engines do not share a stream. The Python half of that claim is
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
