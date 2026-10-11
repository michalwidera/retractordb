"""Descriptor reading - the part of the binding with no I/O behind it."""

from __future__ import annotations

from pathlib import Path

import pytest


def test_load_descriptor_reads_fields(rdb, plain_storage: Path) -> None:
    desc = rdb.load_descriptor(str(plain_storage / "plain_file.desc"))

    assert len(desc) == 2
    assert [field.name for field in desc] == ["a", "b"]
    assert [field.type for field in desc] == [rdb.FieldType.BYTE, rdb.FieldType.BYTE]
    assert desc.size_bytes == 2
    assert desc.flat_element_count == 2


def test_descriptor_lookups(rdb, plain_storage: Path) -> None:
    desc = rdb.load_descriptor(str(plain_storage / "plain_file.desc"))

    assert desc.has_field("a")
    assert not desc.has_field("nonexistent")
    assert desc.field_index("b") == 1
    assert desc.byte_offset("a") == 0
    assert desc.byte_offset("b") == 1
    assert desc.field_type_name("a") == "BYTE"


def test_descriptor_indexing_follows_the_sequence_protocol(rdb, plain_storage: Path) -> None:
    desc = rdb.load_descriptor(str(plain_storage / "plain_file.desc"))

    assert desc[0].name == "a"
    assert desc[-1].name == "b"
    with pytest.raises(IndexError):
        desc[2]


def test_missing_descriptor_raises_instead_of_ending_the_process(rdb, tmp_path: Path) -> None:
    """The guard that matters most in a notebook: a wrong path must not be fatal."""
    with pytest.raises(rdb.NoSuchStream):
        rdb.load_descriptor(str(tmp_path / "absent.desc"))
