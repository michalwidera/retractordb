"""Kernel survival: what a bad descriptor does to a host process, pinned as tests.

**Core phase 1, slice 1 has landed for the descriptor read path.**
``loadDescriptorFile`` now throws ``rdb::CorruptDescriptor`` and the DESC parser's
error listeners throw instead of calling ``exit(EPERM)``
(``src/rdb/lib/descriptorIO.cc``, ``src/rdb/lib/DESCParser.cc``), so the two cases
below run **in this interpreter** and assert an exception. That the rest of the
suite still runs afterwards is the assertion that matters: before the slice, each
of these took the interpreter down with ``std::exit``, which no ``except`` can see.

**Sub-slice 2a has landed too**, closing the path that *builds* a storage:
``storagePaths``, ``accessorFactory`` and ``storage::attachDescriptor`` now raise
``ConfigError`` (bad input) or ``InternalError`` (broken engine invariant).

What has *not* changed: the ~60 remaining ``FatalError`` sites on the **read and
write** path - ``storage::read``/``revRead``/``write``, ``payload``, ``fagrp``,
``facc*`` - still end the process. ``test_guarded_paths_do_not_end_the_process`` is
what keeps the binding-level guards in front of them from being removed early; it
still runs in a subprocess, because the thing it guards against would otherwise kill
this one.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest


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


def test_empty_descriptor_file_raises(rdb, tmp_path: Path) -> None:
    """descriptorIO.cc - an empty descriptor is CorruptDescriptor, not std::exit."""
    path = tmp_path / "empty.desc"
    path.write_text("{\n}\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.load_descriptor(str(path))


def test_malformed_descriptor_file_raises(rdb, tmp_path: Path) -> None:
    """DESCParser.cc - the ANTLR error listeners throw; the parse failure reaches here."""
    path = tmp_path / "malformed.desc"
    path.write_text("this is not a descriptor at all\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.load_descriptor(str(path))


def test_corrupt_descriptor_is_a_retractordb_error(rdb, tmp_path: Path) -> None:
    """The new type belongs to the hierarchy, so `except RetractorDBError` still catches it.

    Worth pinning separately: nanobind tries exception translators in reverse
    registration order, and ``rdb::CorruptDescriptor`` derives from
    ``std::runtime_error`` - which nanobind's own fallback translator would turn
    into a plain ``RuntimeError`` if it were reached first.
    """
    path = tmp_path / "malformed.desc"
    path.write_text("nonsense\n", encoding="utf-8")

    with pytest.raises(rdb.RetractorDBError):
        rdb.load_descriptor(str(path))

    assert issubclass(rdb.CorruptDescriptor, rdb.RetractorDBError)


def test_a_bad_descriptor_does_not_poison_the_next_read(rdb, plain_storage: Path) -> None:
    """The Python-side twin of the parser's no-poison test.

    ``statusDesc`` used to be a file-scope global that nothing reset on entry. It
    was harmless only because ``exit(EPERM)`` came first; with the exit gone, a
    sticky "Fail" would make every later descriptor in the process unreadable -
    the exact "works the first time, fails on re-run" shape that makes a notebook
    infuriating.
    """
    bad = plain_storage / "bad.desc"
    bad.write_text("not a descriptor\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.load_descriptor(str(bad))

    good = rdb.load_descriptor(str(plain_storage / "plain_file.desc"))
    assert good.size_bytes > 0
    assert [field.name for field in good] == ["a", "b"]


def test_storage_open_survives_a_corrupt_descriptor(rdb, plain_storage: Path) -> None:
    """The throw has to unwind through storage's constructor and destructor.

    ``Storage(...)`` reaches ``loadDescriptorFile`` inside ``attachDescriptor()``,
    one frame below a half-built ``storage`` that owns ``unique_ptr`` members and a
    destructor which flushes a pending gap. This is the test that would catch that
    path unwinding badly rather than merely reporting.
    """
    (plain_storage / "broken.desc").write_text("garbage\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.Storage("broken", "broken", storage_param=str(plain_storage))

    # The interpreter is still here, and an unrelated storage still opens.
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as storage:
        assert len(storage) == 4


def test_unsupported_storage_type_raises(rdb, plain_storage: Path) -> None:
    """The hole sub-slice 2a closed, and the reason it was a hole.

    ``storage_type`` reaches ``makeAccessor``, whose list of accepted types is the
    only place that list exists - so no guard in the binding could stand in front of
    it without duplicating that list and drifting from it. Before 2a this call took
    the kernel down.
    """
    with pytest.raises(rdb.ConfigError) as caught:
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), storage_type="NONSENSE")

    # The message has to name what was allowed; the list is not discoverable otherwise.
    assert "NONSENSE" in str(caught.value)
    assert "TEXTSOURCE" in str(caught.value)


def test_config_errors_share_the_base(rdb, tmp_path: Path) -> None:
    """ConfigError and InternalError hang off RetractorDBError, like CorruptDescriptor.

    Same reasoning as the CorruptDescriptor case: the C++ types derive from
    std::runtime_error, so a missing registration shows up as a bare RuntimeError
    rather than as a member of the hierarchy.
    """
    assert issubclass(rdb.ConfigError, rdb.RetractorDBError)
    assert issubclass(rdb.InternalError, rdb.RetractorDBError)


def test_storage_survives_a_rejected_type_and_opens_afterwards(rdb, plain_storage: Path) -> None:
    """A rejected configuration must leave nothing behind - including the fixture files.

    ``storage``'s destructor deletes the whole file set when the storage is disposable,
    so a construction that throws part-way is exactly where a stray delete would show
    up. It also has to leave the process able to open the same storage properly.
    """
    with pytest.raises(rdb.ConfigError):
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), storage_type="NONSENSE")

    assert (plain_storage / "plain_file.desc").exists()
    assert (plain_storage / "plain_file").exists()

    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as storage:
        assert len(storage) == 4


def test_guarded_paths_do_not_end_the_process(module_root: Path, tmp_path: Path) -> None:
    """The counterpart: everything the binding guards must stay survivable.

    This is the test that would notice a guard being dropped from module.cpp while
    the FatalError site behind it is still there. Unlike the cases above, it is
    expected to keep passing forever, and it keeps its subprocess because a dropped
    guard kills the interpreter that runs it.
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
