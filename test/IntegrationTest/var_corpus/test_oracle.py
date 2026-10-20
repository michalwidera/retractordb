#!/usr/bin/env python3
"""Samotest: wyrocznia musi odrzucac uszkodzone i niepelne dowody."""
import copy
import struct
import tempfile
import unittest
from pathlib import Path

import generate
import oracle as o


class OracleTest(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.result = dict(model=dict(types={"s": ["INTEGER", "FLOAT"]}, sources={}, rules=[]),
                           rows={"s": [[o.I(7), None], [o.I(9), o.F(2)]]}, fired={})
        (self.root / "s").write_bytes(struct.pack("=ifif", 7, 0, 9, 2))
        (self.root / "s.desc").write_text("{ INTEGER s_0 FLOAT s_1 }")
        size = "Q" if struct.calcsize("P") == 8 else "I"
        self.entry = struct.Struct("=B" + size * 2 + "B")
        (self.root / "s.meta").write_bytes(bytes(8) + self.entry.pack(0, 1, 2, 2) + self.entry.pack(0, 1, 2, 0))

    def test_complete_storage(self):
        o.check_storage(self.root, self.result)

    def test_record_count_and_order(self):
        original = (self.root / "s").read_bytes()
        for broken in (original[:8], original + original[-8:], original[8:] + original[:8]):
            with self.subTest(content=broken), self.assertRaises(ValueError):
                (self.root / "s").write_bytes(broken)
                o.check_storage(self.root, self.result)

    def test_float_bits_and_partial_record(self):
        original = (self.root / "s").read_bytes()
        for broken in (original[:12] + struct.pack("=f", 2.000000238418579), original[:-1]):
            with self.subTest(content=broken), self.assertRaises(ValueError):
                (self.root / "s").write_bytes(broken)
                o.check_storage(self.root, self.result)

    def test_null_bits_and_invalid_metadata(self):
        for content in (bytes(8), bytes(8) + self.entry.pack(0, 2, 2, 0),
                        bytes(8) + self.entry.pack(1, 2, 2, 2),
                        bytes(8) + self.entry.pack(0, 2, 3, 2),
                        bytes(8) + self.entry.pack(0, 3, 2, 2), bytes(9)):
            with self.subTest(content=content), self.assertRaises(ValueError):
                (self.root / "s.meta").write_bytes(content)
                o.check_storage(self.root, self.result)

    def test_missing_and_wrong_descriptor(self):
        (self.root / "s.desc").unlink()
        with self.assertRaises(FileNotFoundError):
            o.check_storage(self.root, self.result)
        (self.root / "s.desc").write_text("{ INTEGER s_0 INTEGER s_1 }")
        with self.assertRaises(ValueError):
            o.check_storage(self.root, self.result)

    def test_extra_stream_and_dump(self):
        for name in ("foreign.desc", "foreign_rule_dump.tmp"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                path = self.root / name
                path.touch()
                try:
                    o.check_storage(self.root, self.result)
                finally:
                    path.unlink()

    def test_dump_payload_count_and_order(self):
        result = copy.deepcopy(self.result)
        result["model"]["rules"] = [("event", "s", o.ALWAYS, ("DUMP", 0, 1))]
        result["fired"] = {"event": [1]}
        path = self.root / "s_event_dump.tmp"
        with self.assertRaises(ValueError):
            o.check_storage(self.root, result)
        path.write_bytes(struct.pack("=if", 9, 2))
        o.check_storage(self.root, result)
        for broken in (b"", struct.pack("=if", 7, 2), struct.pack("=ifif", 9, 2, 9, 2)):
            with self.subTest(content=broken), self.assertRaises(ValueError):
                path.write_bytes(broken)
                o.check_storage(self.root, result)

    def test_saved_inputs_and_patterns_are_reproducible(self):
        for case in generate.CASES:
            directory = generate.HERE / case
            for filename, rows in generate.datasets(case).items():
                self.assertEqual((directory / filename).read_text(), generate.encoded(rows), (case, filename))
            result = o.simulate(case, directory)
            o.validate_events(case, result)
            self.assertEqual((directory / "expected.jsonl").read_text(), o.expected_text(result), case)

    def test_hand_calculated_slot_boundaries(self):
        root = generate.HERE
        rows = o.simulate("uc10", root / "uc10")["rows"]["status"]
        self.assertEqual(rows[119][1:], [o.I(0), o.I(0)])
        self.assertEqual(rows[120][1:], [o.I(1), o.I(0)])
        self.assertEqual(rows[160][2], o.I(0))
        rows = o.simulate("uc11", root / "uc11")["rows"]["state"]
        self.assertEqual([rows[i][2] for i in (100, 101, 105, 106, 114, 115)],
                         [o.I(i) for i in (0, 1, 1, 0, 1, 2)])
        rows = o.simulate("uc12", root / "uc12")["rows"]["odi"]
        self.assertIsNone(rows[0][4])
        self.assertEqual(rows[1][4], o.I(0))
        rows = o.simulate("uc15", root / "uc15")["rows"]["line"]
        self.assertEqual([rows[i][2] for i in (19, 20, 21, 29, 30, 59, 60, 69, 70)],
                         [o.I(i) for i in (0, 0, 1, 1, 0, 1, 1, 1, 0)])
        rows = o.simulate("uc16", root / "uc16")["rows"]["bucket"]
        self.assertEqual([r[1] for r in rows[:4]], [o.I(i) for i in (50, 45, 40, 42)])

    def test_lta_really_freezes(self):
        rows = o.simulate("uc13", generate.HERE / "uc13")["rows"]["detector"]
        frozen = [i for i in range(len(rows) - 1) if rows[i][3] == o.I(1)]
        self.assertGreater(len(frozen), 100)
        self.assertTrue(all(rows[i + 1][2] == rows[i][2] for i in frozen))


if __name__ == "__main__":
    unittest.main()
