"""Kernel survival: what a failure in the storage layer does to a host process, pinned as tests.

On the engine side ``FatalError`` (class K1) and ``EnvironmentError`` (K4) end the
process with ``std::exit`` - except inside an ``rdb::ThrowOnFatalScope``, where they
throw ``rdb::EngineFatal`` instead (``src/include/fatalError.hpp``, decision (a) of
#463). Every call this binding makes into the engine opens that scope
(``engineCall`` in ``src/python/module.cpp``), so K1 arrives here as
``InternalError`` and K4 as ``IOError``. Refusals the engine reports by value - a
descriptor that does not parse, ``tryLoadDescriptorFile`` - arrive as
``CorruptDescriptor``.

The cases below therefore run **in this interpreter** and assert an exception. That
the rest of the suite still runs afterwards is the assertion that matters.

The guards in ``module.cpp`` stay as *translation*: an empty argument is a
``ValueError``, an unknown field a ``KeyError``, an out-of-range index an
``IndexError`` - the types a Python caller expects, where the engine would report
K1. ``test_guarded_paths_do_not_end_the_process`` still pins them in a subprocess:
it is cheap, and it notices a dropped guard whose engine path would not be covered.

The full class table (K1-K7) of the embedded API, including which of these
refusals should become ``ConfigError``, belongs to #476.
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
    """descriptorIO.cc - an empty descriptor is a refusal by value, so CorruptDescriptor."""
    path = tmp_path / "empty.desc"
    path.write_text("{\n}\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.load_descriptor(str(path))


def test_malformed_descriptor_file_raises(rdb, tmp_path: Path) -> None:
    """DESCParser.cc - the parse failure comes back as a status and reaches here as CorruptDescriptor."""
    path = tmp_path / "malformed.desc"
    path.write_text("this is not a descriptor at all\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.load_descriptor(str(path))


def test_corrupt_descriptor_is_a_retractordb_error(rdb, tmp_path: Path) -> None:
    """The new type belongs to the hierarchy, so `except RetractorDBError` still catches it.

    Worth pinning separately: nanobind tries exception translators in reverse
    registration order, and the binding's C++ types derive from
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

    A parse state left behind by the failed read would make every later
    descriptor in the process unreadable - the exact "works the first time, fails
    on re-run" shape that makes a notebook infuriating.
    """
    bad = plain_storage / "bad.desc"
    bad.write_text("not a descriptor\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.load_descriptor(str(bad))

    good = rdb.load_descriptor(str(plain_storage / "plain_file.desc"))
    assert good.size_bytes > 0
    assert [field.name for field in good] == ["a", "b"]


def test_storage_open_survives_a_corrupt_descriptor(rdb, plain_storage: Path) -> None:
    """The refusal has to leave a half-built storage that is destroyed cleanly.

    ``Storage(...)`` reaches ``tryLoadDescriptorFile`` inside
    ``attachDescriptorStatus()``; the binding then drops a ``storage`` that owns
    ``unique_ptr`` members and a destructor which flushes a pending gap. This is the
    test that would catch that path ending badly rather than merely reporting.
    """
    (plain_storage / "broken.desc").write_text("garbage\n", encoding="utf-8")

    with pytest.raises(rdb.CorruptDescriptor):
        rdb.Storage("broken", "broken", storage_param=str(plain_storage))

    # The interpreter is still here, and an unrelated storage still opens.
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as storage:
        assert len(storage) == 4


def test_unsupported_storage_type_raises(rdb, plain_storage: Path) -> None:
    """``storage_type`` reaches ``makeAccessor``, whose list of accepted types is the
    only place that list exists - so no guard in the binding can stand in front of
    it without duplicating that list and drifting from it.

    The engine reports an unknown type as K1 (``FatalError``), so in throw mode it
    arrives as ``InternalError``. Without the scope this call took the kernel down.
    Whether a bad argument should be ``ConfigError`` instead is #476.
    """
    with pytest.raises(rdb.InternalError) as caught:
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), storage_type="NONSENSE")

    assert "NONSENSE" in str(caught.value)


def test_engine_errors_share_the_base(rdb, tmp_path: Path) -> None:
    """InternalError and IOError hang off RetractorDBError, like CorruptDescriptor.

    Same reasoning as the CorruptDescriptor case: the C++ types derive from
    std::runtime_error, so a missing registration shows up as a bare RuntimeError
    rather than as a member of the hierarchy.
    """
    assert issubclass(rdb.InternalError, rdb.RetractorDBError)
    assert issubclass(rdb.IOError, rdb.RetractorDBError)


def test_storage_survives_a_rejected_type_and_opens_afterwards(rdb, plain_storage: Path) -> None:
    """A rejected configuration must leave nothing behind - including the fixture files.

    ``storage``'s destructor deletes the whole file set when the storage is disposable,
    so a construction that throws part-way is exactly where a stray delete would show
    up. It also has to leave the process able to open the same storage properly.
    """
    with pytest.raises(rdb.InternalError):
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), storage_type="NONSENSE")

    assert (plain_storage / "plain_file.desc").exists()
    assert (plain_storage / "plain_file").exists()

    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as storage:
        assert len(storage) == 4


def test_unknown_field_raises_key_error(rdb, plain_storage: Path) -> None:
    """A typo in a field name is a normal event in a notebook, not a fatal one.

    The name arrives from whoever typed it. The engine reports an unknown field as K1;
    the binding checks ``has_field`` first and raises the type Python expects for a
    failed lookup.
    """
    desc = rdb.load_descriptor(str(plain_storage / "plain_file.desc"))

    for call in (desc.field_index, desc.byte_offset, desc.field_type_name):
        with pytest.raises(KeyError):
            call("no_such_field")

    assert desc.field_index("a") == 0
    assert desc.has_field("b")


def test_the_storage_layer_has_no_remaining_fatal_paths(rdb, plain_storage: Path) -> None:
    """Sweep the storage-layer failure modes, in one interpreter that has to survive them all.

    Without the throw-mode scope each of these reached a different ``std::exit`` - the
    accessor factory, the descriptor read, the descriptor lookup, the storage
    constructor. The assertion is not any single exception type; it is that the loop
    finishes and the process is still here to report it.
    """
    desc_path = str(plain_storage / "plain_file.desc")
    survived = 0

    cases = (
        lambda: rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), storage_type="NONSENSE"),
        lambda: rdb.load_descriptor(str(plain_storage / "nope.desc")),
        lambda: rdb.load_descriptor(desc_path).field_index("absent"),
        lambda: rdb.Storage("", "x", storage_param=str(plain_storage)),
    )
    for case in cases:
        try:
            case()
        except (rdb.RetractorDBError, KeyError, ValueError):
            survived += 1

    assert survived == len(cases)

    # ...and the engine is still usable afterwards, which a poisoned global would break.
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage)) as storage:
        assert len(storage) == 4
        assert storage[0][0] == 1


def test_guarded_paths_do_not_end_the_process(module_root: Path, tmp_path: Path) -> None:
    """The counterpart: everything the binding guards must stay survivable.

    This is the test that would notice a guard being dropped from module.cpp while
    the engine path behind it still ends the process. Unlike the cases above, it is
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
