#!/usr/bin/env python3
"""Regresje scripts/req-trace.py na syntetycznych drzewach: kazda regula musi umiec zawiesc."""

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


repo = Path(sys.argv.pop(1))
spec = importlib.util.spec_from_file_location("req_trace", repo / "scripts" / "req-trace.py")
req_trace = importlib.util.module_from_spec(spec)
spec.loader.exec_module(req_trace)

# Fikstury skladane z linii w osobnych literalach: zaden wiersz tego pliku nie zaczyna sie
# od znacznika pokrycia, wiec req-trace na prawdziwym drzewie ich nie widzi.
HEADER = ("/// Funkcja f powinna:\n/// - [mod.f.ok~2] robic swoje.\n///   Ograniczenie: opis.\nint f();\n"
          "/// Proza poza blokiem:\n/// - [PUSH_STREAM sq] lista w nawiasie.\n/// - zwykly punkt.\n")
TEST = "TEST(Mod, f_works) {\n  // covers: mod.f.ok~2\n}\n"


class ReqTraceTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="req-trace-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write(self, relative, text):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def errors(self):
        return req_trace.collect(self.root)[2]

    def assertOneError(self, fragment):
        errors = self.errors()
        self.assertEqual(len(errors), 1, errors)
        self.assertIn(fragment, errors[0])

    def test_clean_tree_maps_requirement_to_its_test(self):
        self.write("src/f.hpp", HEADER)
        self.write("test/UnitTest/test_f.cpp", TEST)
        requirements, coverage, errors = req_trace.collect(self.root)
        self.assertEqual(errors, [])
        self.assertEqual(requirements["mod.f.ok"][0], 2)
        self.assertEqual(coverage["mod.f.ok"], ["test/UnitTest/test_f.cpp Mod.f_works"])

    def test_python_and_file_level_markers_count(self):
        self.write("api/f.hpp", HEADER)
        self.write("test/x/run.sh", "#!/bin/bash\n# covers: mod.f.ok~2\n")
        self.write("test/x/check.py", "def test_it():\n    # covers: mod.f.ok~2\n    pass\n")
        coverage = req_trace.collect(self.root)[1]
        self.assertEqual(coverage["mod.f.ok"], ["test/x/check.py test_it", "test/x/run.sh"])

    def test_uncovered_requirement_fails(self):
        self.write("src/f.hpp", HEADER)
        self.assertOneError("nie ma zadnego covers")

    def test_duplicate_id_fails_even_across_versions(self):
        self.write("src/f.hpp", HEADER)
        self.write("src/g.hpp", "/// Funkcja g powinna:\n/// - [mod.f.ok~1] drugi raz.\n")
        self.write("test/UnitTest/test_f.cpp", TEST)
        self.assertOneError("zduplikowane ID mod.f.ok")

    def test_malformed_id_fails(self):
        self.write("src/f.hpp", "/// Obiekt klasy F powinien:\n/// - [Mod.F~1] wielka litera.\n")
        self.assertOneError("ID o zlej skladni")

    def test_dangling_covers_fails(self):
        self.write("src/f.hpp", HEADER)
        self.write("test/UnitTest/test_f.cpp", TEST + "TEST(Mod, other) {\n  // covers: mod.gone~1\n}\n")
        self.assertOneError("nieznane wymaganie mod.gone~1")

    def test_stale_version_fails(self):
        self.write("src/f.hpp", HEADER)
        self.write("test/UnitTest/test_f.cpp", TEST.replace("~2", "~1"))
        errors = self.errors()
        self.assertTrue(any("ma wersje ~2" in error for error in errors), errors)
        self.assertTrue(any("nie ma zadnego covers" in error for error in errors), errors)

    def test_malformed_covers_token_fails(self):
        self.write("src/f.hpp", HEADER)
        self.write("test/UnitTest/test_f.cpp", TEST.replace("mod.f.ok~2", "mod.f.ok~2, mod.f.ok"))
        self.assertOneError("covers z ID o zlej skladni: 'mod.f.ok'")

    def test_block_bullet_without_id_fails(self):
        self.write("src/f.hpp", HEADER.replace("/// - [mod.f.ok~2] ", "/// - "))
        self.assertOneError("punkt wymagania bez ID")

    def test_old_test_line_next_to_an_id_fails(self):
        self.write("src/f.hpp", HEADER.replace("///   Ograniczenie: opis.", "/// (wtracona notka)\n///   Test: `Mod.f_works`."))
        self.write("test/UnitTest/test_f.cpp", TEST)
        self.assertOneError("linia Test: przy punkcie z ID")

    def test_untestable_requirement_needs_no_covers(self):
        for marker in ("Nietestowalne", "Kontrola strukturalna"):
            with self.subTest(marker=marker):
                self.write("src/f.hpp", HEADER.replace("Ograniczenie", marker))
                requirements, coverage, errors = req_trace.collect(self.root)
                self.assertEqual(errors, [])
                self.assertTrue(requirements["mod.f.ok"][2])

    def test_zone_ends_with_the_block(self):
        # Linia "Nietestowalne" po kodzie nalezy juz do innego komentarza, nie zwalnia punktu.
        self.write("src/f.hpp", HEADER + "int g();\n///   Nietestowalne: cudze.\n")
        self.assertOneError("nie ma zadnego covers")

    def test_build_directories_are_ignored(self):
        self.write("src/f.hpp", HEADER)
        self.write("test/UnitTest/test_f.cpp", TEST)
        self.write("test/build/copy.hpp", HEADER)
        self.write("src/build/copy.hpp", HEADER)
        self.assertEqual(self.errors(), [])


if __name__ == "__main__":
    unittest.main()
