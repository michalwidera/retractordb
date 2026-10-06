#!/usr/bin/env python3
"""Regresje kolektora na syntetycznych katalogach, bez uruchamiania CI."""

import importlib.util
import io
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch


repo = Path(sys.argv.pop(1))
spec = importlib.util.spec_from_file_location(
    "collector", repo / "scripts" / "collect-test-failures.py")
collector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(collector)


class CollectorTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="collect-test-failures-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.work = self.root / "work"
        self.work.mkdir()
        self.report = self.root / "report"
        self.write("pattern.txt", "expected\n")
        self.write("out.txt", "foreign output\n")

    def write(self, name, text):
        (self.work / name).write_text(text, encoding="utf-8")

    def collect(self, command, name="it_sample-run"):
        lines = collector.collectOne(
            name, {"workingDirectory": str(self.work), "command": command},
            "", self.report)
        target = self.report / name
        diffs = {p.name: p.read_text(encoding="utf-8") for p in target.glob("diff-*")}
        return "\n".join(lines), diffs

    def test_valgrind_without_comparison_does_not_guess_pair(self):
        summary, diffs = self.collect(
            ["valgrind", "xretractor", "-q", "query.rql"], "it_sample-vg-run")
        self.assertEqual(diffs, {})
        self.assertNotIn("roznica", summary)

    def test_script_without_explicit_comparison_does_not_guess_pair(self):
        _, diffs = self.collect(["bash", "run.sh"])
        self.assertEqual(diffs, {})

    def test_missing_output_is_reported_without_foreign_diff(self):
        command = "bash ../compare.sh --ignore-eol pattern.txt missing.txt"
        self.assertEqual(collector.comparePairs(command, self.work),
                         [(self.work / "pattern.txt", self.work / "missing.txt")])
        summary, diffs = self.collect(["bash", "-c", command])
        self.assertEqual(set(diffs), {"diff-pattern.txt-vs-missing.txt.txt"})
        self.assertIn("missing.txt", summary)
        self.assertIn("nie zostal wytworzony", summary)
        self.assertIn("nie zostal wytworzony", next(iter(diffs.values())))
        self.assertNotIn("foreign output", next(iter(diffs.values())))
        self.assertNotIn("brak roznic tekstowych", summary)

    def test_missing_pattern_is_reported(self):
        summary, diffs = self.collect(
            ["bash", "-c", "bash ../compare.sh absent.pattern out.txt"])
        self.assertEqual(set(diffs), {"diff-absent.pattern-vs-out.txt.txt"})
        self.assertIn("wzorzec absent.pattern: BRAK", summary)
        self.assertNotIn("brak roznic tekstowych", next(iter(diffs.values())))

    def test_explicit_comparison_produces_correct_diff(self):
        self.write("actual.txt", "actual output\n")
        summary, diffs = self.collect(
            ["bash", "-c", "bash ../compare.sh --ignore-eol pattern.txt actual.txt"])
        self.assertEqual(set(diffs), {"diff-pattern.txt-vs-actual.txt.txt"})
        diff = next(iter(diffs.values()))
        self.assertIn("-expected\n", diff)
        self.assertIn("+actual output\n", diff)
        self.assertNotIn("foreign output", diff)
        self.assertIn("roznica pattern.txt vs actual.txt: zapisana", summary)

    def test_multiple_explicit_pairs_stay_separate(self):
        self.write("actual.txt", "expected\n")
        self.write("pattern-dot.txt", "expected graph\n")
        self.write("out.dot", "actual graph\n")
        _, diffs = self.collect([
            "bash", "-c",
            "bash ../compare.sh pattern.txt actual.txt && "
            "bash ../compare.sh --ignore-eol pattern-dot.txt out.dot"])
        self.assertEqual(set(diffs), {"diff-pattern.txt-vs-actual.txt.txt",
                                     "diff-pattern-dot.txt-vs-out.dot.txt"})
        self.assertEqual(diffs["diff-pattern.txt-vs-actual.txt.txt"],
                         "(brak roznic tekstowych)\n")
        self.assertIn("+actual graph\n", diffs["diff-pattern-dot.txt-vs-out.dot.txt"])

    def test_log_is_preserved_before_catalogue_overwrites_it(self):
        testing = self.root / "Testing" / "Temporary"
        testing.mkdir(parents=True)
        (testing / "LastTestsFailed.log").write_text("1:it_sample-run\n")
        log = "1/1 Testing: it_sample-run\noriginal evidence\nTest Failed.\n"
        (testing / "LastTest.log").write_text(log)
        self.report.mkdir()
        (self.report / "SUMMARY.txt").write_text("old report\n")
        (self.report / "old-diff.txt").write_text("stale evidence\n")

        def catalogue(test_dir):
            self.assertEqual(test_dir, self.root)
            (testing / "LastTest.log").write_text("overwritten by ctest\n")
            return {"it_sample-run": {"workingDirectory": str(self.work),
                                      "command": ["bash", "run.sh"]}}

        with patch.object(collector, "testCatalogue", side_effect=catalogue), \
                patch.object(sys, "argv", ["collector", str(self.root), str(self.report),
                                           "--ctest-status", "8"]), \
                redirect_stdout(io.StringIO()):
            self.assertEqual(collector.main(), 0)
        self.assertEqual((self.report / "LastTest.log").read_text(), log)
        self.assertEqual((self.report / "it_sample-run" / "ctest-output.log").read_text(), log)
        self.assertFalse((self.report / "old-diff.txt").exists())

    def test_green_run_discards_stale_report(self):
        self.report.mkdir()
        (self.report / "SUMMARY.txt").write_text("old report\n")
        with patch.object(sys, "argv", ["collector", str(self.root), str(self.report),
                                       "--ctest-status", "0"]), \
                patch.object(collector, "testCatalogue") as catalogue, \
                redirect_stdout(io.StringIO()):
            self.assertEqual(collector.main(), 0)
        catalogue.assert_not_called()
        self.assertFalse(self.report.exists())

    def test_passed_test_in_stale_failure_list_is_ignored(self):
        log = "1/1 Testing: it_sample-run\nTest Passed.\n"
        self.assertFalse(collector.failedInThisRun(log, "it_sample-run"))


if __name__ == "__main__":
    unittest.main()
