"""Error classes K1-K7 of the embedded engine, as a Python caller meets them (#476).

The table lives in docs/jupyter-integration.md section 3. Each test below pins one row:
the exception (or warning) and the state the instance is left in.

K1 taints the whole module until the interpreter restarts, so the tests that provoke
it run in a subprocess - a K1 here would fail every test after it. That subprocess is
also the "kernel survives" check: it has to print ``alive`` after the error.
"""

from __future__ import annotations

import errno
import os
import subprocess
import sys
import warnings
from pathlib import Path

import pytest

INPUT = [10, 20, 30, 40]


def doubling_plan(source: Path) -> str:
    return f"DECLARE a INTEGER STREAM src, 1/2 TEXTFILE '{source}'\nSELECT a*2 STREAM dst FROM src\n"


@pytest.fixture
def plan_dir(tmp_path: Path) -> Path:
    (tmp_path / "data.txt").write_text("".join(f"{value}\n" for value in INPUT))
    return tmp_path


def run_survivor(module_root: Path, plan_dir: Path, body: str) -> subprocess.CompletedProcess[str]:
    """Run ``body`` in a fresh interpreter with ``engine`` holding a compiled doubling plan."""
    program = "\n".join(
        [
            "import os, sys",
            f"sys.path.insert(0, {str(module_root)!r})",
            "import retractordb as rdb",
            f"plan_dir = {str(plan_dir)!r}",
            f"plan = {doubling_plan(plan_dir / 'data.txt')!r}",
            "engine = rdb.Engine()",
            "engine.compile(plan)",
            body,
            "print('alive')",
        ]
    )
    return subprocess.run([sys.executable, "-c", program], capture_output=True, text=True, timeout=120, check=False)


def test_k1_internal_error_taints_the_module(module_root: Path, plan_dir: Path) -> None:
    """K1 -> InternalError; every later call into the engine refuses until a restart."""
    body = "\n".join(
        [
            "os.environ['RDB_FAULT_FATAL_IN_SLOT'] = '0'",
            "try:",
            "    engine.step()",
            "    raise SystemExit('K1 did not raise')",
            "except rdb.InternalError as error:",
            "    assert not isinstance(error, (ValueError, OSError)), type(error).__mro__",
            "    assert 'RDB_FAULT_FATAL_IN_SLOT' in str(error), error",
            "del os.environ['RDB_FAULT_FATAL_IN_SLOT']",
            "assert engine.failed",
            "for call in (lambda: engine.step(), lambda: engine.compile(plan), lambda: rdb.Engine(plan_dir),",
            "             lambda: engine.record_count('dst')):",
            "    try:",
            "        call()",
            "        raise SystemExit('a tainted module accepted a call')",
            "    except rdb.InternalError as error:",
            "        assert 'restart the interpreter' in str(error), error",
            "        assert 'RDB_FAULT_FATAL_IN_SLOT' in str(error), error",
            "engine.close()",
        ]
    )

    result = run_survivor(module_root, plan_dir, body)

    assert result.returncode == 0, f"stdout: {result.stdout}\nstderr: {result.stderr}"
    assert "alive" in result.stdout


def test_kernel_survives_k2_k3_k4_and_k1(module_root: Path, plan_dir: Path) -> None:
    """One interpreter, every recoverable class in turn, K1 last: the process is still here."""
    body = "\n".join(
        [
            "try:",
            "    engine.compile('SELECT nonsense')",
            "except rdb.PlanError:",
            "    pass",
            "try:",
            "    engine.step()",
            "except rdb.ConfigError:",
            "    pass",
            "engine.compile(plan)",
            "os.environ['RDB_FAULT_ENV_IN_SLOT'] = '1'",
            "try:",
            "    engine.step()",
            "except rdb.IOError:",
            "    pass",
            "del os.environ['RDB_FAULT_ENV_IN_SLOT']",
            "engine.compile(plan)",
            "os.environ['RDB_FAULT_FATAL_IN_SLOT'] = '0'",
            "try:",
            "    engine.step()",
            "except rdb.InternalError:",
            "    pass",
        ]
    )

    result = run_survivor(module_root, plan_dir, body)

    assert result.returncode == 0, f"stdout: {result.stdout}\nstderr: {result.stderr}"
    assert "alive" in result.stdout


def test_k2_plan_error_leaves_the_engine_usable(rdb, plan_dir: Path) -> None:
    """K2 -> PlanError (a ValueError); no plan, and compile() works again."""
    with rdb.Engine() as engine:
        with pytest.raises(rdb.PlanError) as failure:
            engine.compile("SELECT nonsense")
        assert isinstance(failure.value, ValueError)
        assert not engine.has_plan

        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        assert [row[0] for row in engine.rows("dst")] == [value * 2 for value in INPUT]


def test_k3_config_error_leaves_the_plan_unchanged(rdb, plan_dir: Path) -> None:
    """K3 -> ConfigError (a ValueError, not a PlanError); the plan goes on where it was."""
    with rdb.Engine() as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run(slots=2)
        before = (engine.slots_done, engine.record_count("dst"))

        with pytest.raises(rdb.ConfigError) as failure:
            engine._block("dst", [99], 0, 1, "float64")
        assert isinstance(failure.value, ValueError)
        assert not isinstance(failure.value, rdb.PlanError)

        assert (engine.slots_done, engine.record_count("dst")) == before
        assert not engine.failed
        engine.run()
        assert [row[0] for row in engine.rows("dst")] == [value * 2 for value in INPUT]


def test_k4_io_error_carries_errno_and_stops_the_plan(rdb, plan_dir: Path) -> None:
    """K4 -> IOError, an OSError with errno; the plan is dead, what it wrote is readable."""
    with rdb.Engine() as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run(slots=2)
        written = [row[0] for row in engine.rows("dst")]

        os.environ["RDB_FAULT_ENV_IN_SLOT"] = "1"
        try:
            with pytest.raises(OSError) as failure:
                engine.step()
        finally:
            del os.environ["RDB_FAULT_ENV_IN_SLOT"]
        assert isinstance(failure.value, rdb.IOError)
        assert failure.value.errno == errno.EIO

        assert engine.failed
        assert [row[0] for row in engine.rows("dst")] == written
        with pytest.raises(rdb.IOError):
            engine.step()


def test_k4_refusal_keeps_the_system_errno_and_the_path(rdb, plain_storage: Path) -> None:
    """A refusal by value keeps its errno, and the binding adds the path it was given.

    A symlink in place of a .desc is refused by the storage layer (O_NOFOLLOW, #374) with
    ELOOP - unlike file permissions, that also holds for root in a CI container.
    """
    link = plain_storage / "link.desc"
    link.symlink_to(plain_storage / "plain_file.desc")

    with pytest.raises(OSError) as failure:
        rdb.load_descriptor(str(link))

    assert isinstance(failure.value, rdb.IOError)
    assert failure.value.errno == errno.ELOOP
    assert failure.value.filename == str(link)


def test_k5_bad_token_is_null_and_the_plan_goes_on(rdb, plan_dir: Path) -> None:
    """K5 -> NULL in the result, no exception; the records around it are intact."""
    (plan_dir / "data.txt").write_text("10\nabc\n30\n40\n")
    with rdb.Engine() as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        assert [row[0] for row in engine.rows("dst")] == [20, None, 60, 80]
        assert not engine.failed


def test_k6_deprecated_forms_are_deprecation_warnings(rdb, plan_dir: Path) -> None:
    """K6 -> DeprecationWarning from compile(), one per deprecated form; the plan runs.

    The subject of this test is the deprecated syntax itself, so it is written here on purpose.
    """
    source = plan_dir / "data.txt"
    plan = f"DECLARE a INTEGER STREAM src, 1/2 FILE '{source}'\nSELECT * STREAM mean FROM src.avg\n"

    with rdb.Engine() as engine:
        with pytest.warns(DeprecationWarning) as caught:
            engine.compile(plan)
        messages = [str(item.message) for item in caught]
        assert len(messages) == 2, messages
        assert "FILE" in messages[0] and "deprecated" in messages[0]
        assert "src.avg" in messages[1] and "AVG(src)" in messages[1]

        engine.run()
        assert engine.record_count("mean") > 0

        # Plan bez form przestarzalych nie ostrzega.
        with warnings.catch_warnings():
            warnings.simplefilter("error")
            engine.compile(doubling_plan(source))
