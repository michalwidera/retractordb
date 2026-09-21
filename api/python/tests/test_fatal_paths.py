"""Kernel survival: what FatalError does to a host process, pinned as tests.

``FatalError`` finishes with ``std::exit(EXIT_FAILURE)``
(``src/include/fatalError.hpp:51``); the DESC parser's error listeners call
``exit(EPERM)`` directly (``src/rdb/lib/DESCParser.cc:25,37``). Neither unwinds,
so no ``catch`` in the binding and no ``except`` in Python can see them. In a
notebook this is not an error message - it is a dead kernel and a lost session.

**These tests assert today's behaviour on purpose.** They are the acceptance
criteria for phase 1 of the shared refactor, written before the work rather than
after it. Each one names the assertion it becomes once those sites throw. When
phase 1 lands and these start failing, that is the refactor succeeding: replace
the body with the ``pytest.raises`` form given in the comment.

Every case runs in a subprocess. It has to - the thing under test kills the
interpreter that runs it, which would otherwise take the whole suite down.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

# Kod wyjscia przy FatalError to EXIT_FAILURE, a przy bledzie skladni deskryptora
# EPERM - obie wartosci to 1 na Linuksie i macOS. Sprawdzamy "niezerowy", bo
# istotne jest to, ze proces ZGINAL, a nie ktorym numerem.
PROCESS_DIED = "the child process must die; if it survived, phase 1 has landed - see this module's docstring"


def run_in_subprocess(root: Path, body: str) -> subprocess.CompletedProcess[str]:
    """Run a snippet against the built module in a throwaway interpreter."""
    program = f"import sys\nsys.path.insert(0, {str(root)!r})\nimport retractordb as rdb\n{body}\n"
    return subprocess.run(
        [sys.executable, "-c", program],
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def test_empty_descriptor_file_ends_the_process(module_root: Path, tmp_path: Path) -> None:
    """descriptorIO.cc:21-25 - an empty descriptor is FatalError, not an exception.

    After phase 1, with a CorruptDescriptor added to the error hierarchy:
        with pytest.raises(rdb.CorruptDescriptor):
            rdb.load_descriptor(str(path))
    """
    path = tmp_path / "empty.desc"
    path.write_text("{\n}\n", encoding="utf-8")

    result = run_in_subprocess(module_root, f"rdb.load_descriptor({str(path)!r})")

    assert result.returncode != 0, PROCESS_DIED
    assert "Traceback" not in result.stderr, (
        "a Python traceback means the failure was an exception, which is what phase 1 "
        "is meant to produce - update this test rather than the engine"
    )


def test_malformed_descriptor_file_ends_the_process(module_root: Path, tmp_path: Path) -> None:
    """DESCParser.cc:25,37 - the ANTLR error listeners call exit(EPERM).

    After phase 1, with a CorruptDescriptor added to the error hierarchy:
        with pytest.raises(rdb.CorruptDescriptor):
            rdb.load_descriptor(str(path))
    """
    path = tmp_path / "malformed.desc"
    path.write_text("this is not a descriptor at all\n", encoding="utf-8")

    result = run_in_subprocess(module_root, f"rdb.load_descriptor({str(path)!r})")

    assert result.returncode != 0, PROCESS_DIED
    assert "Traceback" not in result.stderr


def test_guarded_paths_do_not_end_the_process(module_root: Path, tmp_path: Path) -> None:
    """The counterpart: everything the binding can guard must stay survivable.

    This is the test that would notice a guard being dropped from module.cpp.
    Unlike the two above, it is expected to keep passing forever.
    """
    body = "\n".join(
        [
            "survived = 0",
            f"for call in (lambda: rdb.load_descriptor({str(tmp_path / 'absent.desc')!r}),",
            f"             lambda: rdb.Storage('x', 'x', storage_param={str(tmp_path / 'absent')!r}),",
            f"             lambda: rdb.Storage('', 'x', storage_param={str(tmp_path)!r})):",
            "    try:",
            "        call()",
            "    except (rdb.RetractorDBError, ValueError):",
            "        survived += 1",
            "assert survived == 3, survived",
            "print('alive')",
        ]
    )

    result = run_in_subprocess(module_root, body)

    assert result.returncode == 0, f"a guarded path killed the interpreter: {result.stderr}"
    assert "alive" in result.stdout
