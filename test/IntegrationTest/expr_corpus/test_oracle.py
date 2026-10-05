#!/usr/bin/env python3
"""Kontrole wyroczni #325: uszkodzony lub pusty dowod nie moze byc zielony."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ORACLE = Path(__file__).with_name("oracle.py")
PLAN = """DECLARE x INTEGER, y INTEGER STREAM src, 1 TEXTFILE 'data.txt'
SELECT src[0], src[1] STREAM dst FROM src
"""
ROWS = "== dst\n{ dst_0:1 dst_1:2 }\n{ dst_0:3 dst_1:4 }\n"
DESC = "== desc dst\n{ INTEGER dst_0 INTEGER dst_1 }\n"
OUTPUT = ROWS + DESC + "== dumps\n"


class OracleControls(unittest.TestCase):
    def run_oracle(self, plan=PLAN, data="1 2\n3 4\n", output=OUTPUT):
        with tempfile.TemporaryDirectory(prefix="oracle-control-") as directory:
            base = Path(directory)
            (base / "plan.rql").write_text(plan)
            (base / "data.txt").write_text(data)
            (base / "result.out").write_text(output)
            return subprocess.run([sys.executable, str(ORACLE), str(base / "plan.rql"), str(base / "result.out")],
                                  capture_output=True, text=True, timeout=10)

    def assert_rejected(self, diagnostic, **kwargs):
        result = self.run_oracle(**kwargs)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(diagnostic, result.stdout)
        self.assertNotIn("zgodny", result.stdout)

    def test_complete_comparison(self):
        result = self.run_oracle()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("zgodny, 4 wartosci", result.stdout)

    def test_missing_and_extra_source_columns(self):
        for data in ("1\n3 4\n", "1 2\n3 4 5\n"):
            with self.subTest(data=data):
                self.assert_rejected("kolumn, oczekiwane 2", data=data)

    def test_missing_and_extra_result_columns(self):
        for row in ("{ dst_0:3 }", "{ dst_0:3 dst_1:4 dst_2:5 }"):
            with self.subTest(row=row):
                self.assert_rejected("kolumn wyniku, oczekiwane 2",
                                     output=OUTPUT.replace("{ dst_0:3 dst_1:4 }", row))

    def test_truncated_result_without_descriptor(self):
        # Potwierdzony kontrprzyklad: dawniej jedna porownana wartosc wystarczala do sukcesu.
        self.assert_rejected("kolumn wyniku, oczekiwane 2", data="1 2\n",
                             output="== dst\n{ dst_0:1 }\n== dumps\n")

    def test_missing_and_extra_result_rows(self):
        for rows in (ROWS.replace("{ dst_0:3 dst_1:4 }\n", ""), ROWS + "{ dst_0:5 dst_1:6 }\n"):
            with self.subTest(rows=rows):
                self.assert_rejected("rekordow, wyrocznia oczekuje 2", output=rows + DESC)

    def test_missing_output_stream(self):
        self.assert_rejected("brak wypisu strumienia", output=DESC)

    def test_missing_descriptor(self):
        self.assert_rejected("brak .desc", output=ROWS)

    def test_invalid_descriptors(self):
        for descriptor in ("", "{}", "{ INTEGER dst_0 INTEGER dst_1", "{ INTEGER dst_0 UNKNOWN dst_1 }",
                           "{ INTEGER dst_0 INTEGER dst_1 } blad odczytu"):
            with self.subTest(descriptor=descriptor):
                self.assert_rejected("niepoprawny lub pusty .desc", output=ROWS + "== desc dst\n" + descriptor)

    def test_missing_and_extra_descriptor_types(self):
        for descriptor in ("{ INTEGER dst_0 }", "{ INTEGER dst_0 INTEGER dst_1 INTEGER dst_2 }"):
            with self.subTest(descriptor=descriptor):
                self.assert_rejected("typow w .desc, oczekiwane 2", output=ROWS + "== desc dst\n" + descriptor)

    def test_wrong_descriptor_type(self):
        self.assert_rejected("typ w .desc FLOAT", output=OUTPUT.replace("INTEGER dst_1", "FLOAT dst_1"))

    def test_wrong_value(self):
        self.assert_rejected("silnik 5, wyrocznia INTEGER(4)", output=OUTPUT.replace("dst_1:4", "dst_1:5"))

    def test_duplicate_sections(self):
        for output in (ROWS + OUTPUT, OUTPUT + DESC):
            with self.subTest(output=output):
                self.assert_rejected("powtorzon", output=output)

    def test_empty_and_unrecognized_plan(self):
        for plan in ("", "# pusty plan\n", "UNKNOWN\n", PLAN.splitlines()[0] + "\n", PLAN + "SELECT nonsense\n"):
            with self.subTest(plan=plan):
                self.assert_rejected("WYROCZNIA: blad", plan=plan)

    def test_unrecognized_source(self):
        self.assert_rejected("nierozpoznane", plan=PLAN.replace("TEXTFILE", "UNKNOWN"))

    def test_empty_comparison(self):
        self.assert_rejected("brak wartosci do sprawdzenia", data="", output="== dst\n" + DESC)

    def test_empty_corpus(self):
        with tempfile.TemporaryDirectory(prefix="oracle-no-plans-") as directory:
            result = subprocess.run(["bash", str(ORACLE.with_name("run.sh"))], cwd=directory,
                                    capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("brak planow w korpusie", result.stdout)

    def test_empty_stream_beside_nonempty_stream(self):
        plan = PLAN + "DECLARE x INTEGER, y INTEGER STREAM empty, 1 TEXTFILE 'empty.txt'\n"
        plan += "SELECT empty[0], empty[1] STREAM none FROM empty\n"
        with tempfile.TemporaryDirectory(prefix="oracle-empty-") as directory:
            base = Path(directory)
            (base / "empty.txt").write_text("")
            plan = plan.replace("'empty.txt'", repr(str(base / "empty.txt")))
            self.assert_rejected("none: brak wartosci do sprawdzenia", plan=plan,
                                 output=OUTPUT + "== none\n== desc none\n{ INTEGER none_0 INTEGER none_1 }\n")

    def test_current_avg_syntax(self):
        plan = PLAN.replace("src[0], src[1]", "src[0]").replace("FROM src", "FROM AVG(src)")
        output = "== dst\n{ dst_0:3/2 }\n{ dst_0:7/2 }\n== desc dst\n{ RATIONAL dst_0 }\n"
        result = self.run_oracle(plan=plan, output=output)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("zgodny, 2 wartosci", result.stdout)


if __name__ == "__main__":
    unittest.main()
