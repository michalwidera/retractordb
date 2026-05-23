#!/usr/bin/env python3

from __future__ import annotations

import ctypes
import struct
from pathlib import Path


def write_desc(path: Path, with_retention: bool) -> None:
    lines = ["{", "\tBYTE a", "\tBYTE b"]
    if with_retention:
        # RETENTION <capacity> <segments>
        lines.append("\tRETENTION 3 2")
    lines.append("}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def pack_entry(is_gap: bool, record_count: int, null_bitset: list[bool], size_t_fmt: str) -> bytes:
    bitset_size = len(null_bitset)
    bitset_bytes = bytearray((bitset_size + 7) // 8)
    for i, bit in enumerate(null_bitset):
        if bit:
            bitset_bytes[i // 8] |= 1 << (i % 8)
    return (
        struct.pack("<B", 1 if is_gap else 0)
        + struct.pack(size_t_fmt, record_count)
        + struct.pack(size_t_fmt, bitset_size)
        + bytes(bitset_bytes)
    )


def write_meta(path: Path, field_count: int) -> None:
    # Segments:
    # [====] 2 records, no nulls
    # [----] 1 record, partial null (a null, b non-null)
    # [~~~~] 1 record, all nulls (null fill)
    # [XXXX] 1 record, gap
    entries = [
        pack_entry(False, 2, [False] * field_count, SIZE_T_FMT),
        pack_entry(False, 1, [True, False] + [False] * (field_count - 2), SIZE_T_FMT),
        pack_entry(False, 1, [True, True] + [False] * (field_count - 2), SIZE_T_FMT),
        pack_entry(True, 1, [True] * field_count, SIZE_T_FMT),
    ]

    # Header: int64 creation timestamp placeholder
    payload = struct.pack("<q", 0) + b"".join(entries)
    path.write_bytes(payload)


SIZE_T_FMT = "<Q" if ctypes.sizeof(ctypes.c_size_t) == 8 else "<I"


def main() -> None:
    root = Path(".")

    # Non-retention case
    write_desc(root / "plain_file.desc", with_retention=False)
    # 4 data records x 2 bytes (a,b)
    (root / "plain_file").write_bytes(bytes([1, 10, 2, 20, 3, 30, 4, 40]))
    # one shadow update record: sizeof(size_t)+recordSize = 8+2 = 10 bytes on 64-bit
    shadow_record_size = ctypes.sizeof(ctypes.c_size_t) + 2
    (root / "plain_file.shadow").write_bytes(b"\x00" * shadow_record_size)
    write_meta(root / "plain_file.meta", field_count=2)

    # Retention case (capacity=3, segments=2), same logical 4 records split into two segment files.
    write_desc(root / "ret_file.desc", with_retention=True)
    (root / "ret_file_segment_0").write_bytes(bytes([1, 10, 2, 20, 3, 30]))
    (root / "ret_file_segment_1").write_bytes(bytes([4, 40]))
    (root / "ret_file_segment_0.shadow").write_bytes(b"\x00" * shadow_record_size)
    (root / "ret_file_segment_1.shadow").write_bytes(b"")
    write_meta(root / "ret_file.meta", field_count=3)


if __name__ == "__main__":
    main()
