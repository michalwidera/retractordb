"""The embedded engine end to end: compile, step, read (J1 over core phase 3).

The plans mirror test/IntegrationTest/untileof_stop: eight integers in a text
source, one SELECT doubling them. What the daemon writes for ``xretractor -u``
is what ``Engine.run()`` must leave in the storage directory, record for record.
"""

from __future__ import annotations

import errno
import logging
import os
import subprocess
import sys
import threading
from fractions import Fraction
from pathlib import Path

import pytest

INPUT = [10, 20, 30, 40, 50, 60, 70, 80]


def write_source(directory: Path, values=INPUT, name: str = "data.txt") -> Path:
    path = directory / name
    path.write_text("".join(f"{value}\n" for value in values))
    return path


def doubling_plan(source: Path) -> str:
    # Sciezka bezwzgledna: TEXTFILE jest wzgledne do katalogu roboczego PROCESU, a ten w
    # pytest nie jest katalogiem tymczasowym testu.
    return f"DECLARE a INTEGER STREAM src, 1/2 TEXTFILE '{source}'\nSELECT a*2 STREAM dst FROM src\n"


@pytest.fixture
def plan_dir(tmp_path: Path) -> Path:
    write_source(tmp_path)
    (tmp_path / "storage").mkdir()
    return tmp_path


def test_run_to_end_of_input_doubles_every_record(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        assert not engine.has_plan
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        assert engine.has_plan
        assert engine.streams() == ["src", "dst"]
        assert engine.is_declared("src")
        assert not engine.is_declared("dst")

        done = engine.run()

        assert engine.end_of_input
        assert done == engine.slots_done
        # Ten sam artefakt, ktory zostawia `xretractor -u`: osiem rekordow, zadnego
        # policzonego z all-null wstawionego za koniec pliku.
        assert [row[0] for row in engine.rows("dst")] == [value * 2 for value in INPUT]
        assert engine.record_count("dst") == len(INPUT)
        # Slot 1/2 s: po N slotach czas planu to N/2.
        assert engine.time == Fraction(done, 2)
    assert not engine.has_plan


def test_step_returns_slot_indexes_then_none(rdb, plan_dir: Path) -> None:
    engine = rdb.Engine(str(plan_dir / "storage"))
    engine.compile(doubling_plan(plan_dir / "data.txt"))

    assert engine.step() == 0
    assert engine.step() == 1
    seen = 2
    while engine.step() is not None:
        seen += 1
    assert engine.end_of_input
    assert engine.slots_done == seen
    # Po koncu wejscia kazde kolejne step() odpowiada None i niczego nie liczy.
    assert engine.step() is None
    assert engine.slots_done == seen
    assert [row[0] for row in engine.rows("dst")] == [value * 2 for value in INPUT]
    engine.close()
    engine.close()  # idempotentne


def test_run_with_a_slot_budget_stops_early(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        assert engine.run(slots=3) == 3
        assert engine.slots_done == 3
        assert not engine.end_of_input
        assert engine.run(slots=2) == 2
        assert engine.slots_done == 5


def test_artefacts_land_in_the_engine_storage_dir(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
    assert (plan_dir / "storage" / "dst").exists()
    assert (plan_dir / "storage" / "dst.desc").exists()
    # Magazyn jest zwykly: czyta go tez wiazanie etapu 1a.
    with rdb.Storage("dst", "dst", storage_param=str(plan_dir / "storage")) as storage:
        assert [record[0] for record in storage] == [value * 2 for value in INPUT]


def test_storage_directive_in_the_plan_wins(rdb, plan_dir: Path) -> None:
    inside = plan_dir / "inside"
    inside.mkdir()
    plan = f"STORAGE '{inside}'\n" + doubling_plan(plan_dir / "data.txt")
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(plan)
        engine.run()
    assert (inside / "dst").exists()
    assert not (plan_dir / "storage" / "dst").exists()


def test_missing_storage_dir_is_a_plan_error_not_a_dead_kernel(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "nowhere")) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile(doubling_plan(plan_dir / "data.txt"))
        assert not engine.has_plan


def test_syntax_error_names_the_problem(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        with pytest.raises(rdb.RQLSyntaxError) as failure:
            engine.compile("SELEKT nonsense FROM nowhere\n")
        assert issubclass(rdb.RQLSyntaxError, rdb.PlanError)
        assert str(failure.value)  # tresc parsera, nie pusty napis
        assert not engine.has_plan


def test_compile_error_for_an_unknown_source(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile("SELECT a*2 STREAM dst FROM nosuch\n")
        assert issubclass(rdb.CompileError, rdb.PlanError)


def test_empty_plan_is_a_compile_error(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile("# only a comment\n")


@pytest.mark.parametrize(
    "extra",
    [
        "RULE r ON dst WHEN dst[0] > 100 DO DUMP -1 TO 1\n",
        "RULE r ON dst WHEN dst[0] > 100 DO SYSTEM 'echo no'\n",
        "ROTATION 'counter.txt'\n",
        "DECLARE b INTEGER STREAM dev, 1 DEVICE '/dev/null'\n",
    ],
    ids=["dump-rule", "system-rule", "rotation", "device-source"],
)
def test_service_only_features_are_refused_at_compile(rdb, plan_dir: Path, extra: str) -> None:
    """DUMP needs the service's global model pointer, SYSTEM would run a shell in the host,
    ROTATION reads the service's counter, DEVICE needs the service loop's DEVICE phase.
    Refused up front, not half-way through a slot."""
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile(doubling_plan(plan_dir / "data.txt") + extra)
        assert not engine.has_plan


def test_operations_without_a_plan_raise_config_error(rdb) -> None:
    engine = rdb.Engine()
    with pytest.raises(rdb.ConfigError) as failure:
        engine.step()
    assert not isinstance(failure.value, rdb.PlanError), "a request refusal is not a plan refusal"
    with pytest.raises(rdb.ConfigError):
        engine.streams()
    assert engine.slots_done == 0
    assert engine.time == 0


def test_record_access_follows_python_conventions(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        assert engine.record("dst", -1)[0] == INPUT[-1] * 2
        assert engine.record("dst", -1).index == len(INPUT) - 1
        with pytest.raises(IndexError):
            engine.record("dst", len(INPUT))
        with pytest.raises(KeyError):
            engine.record("typo", 0)
        with pytest.raises(KeyError):
            engine.schema("typo")
        assert [field.name for field in engine.schema("dst")][0] == "dst_0"
        assert engine.retained_from("dst") == 0
        # Deklaracja trzyma tylko ogon historii o pojemnosci z kompilatora; rows() zaczyna od
        # niego. Za koncem pliku zrodlo oddaje rekord all-null, stad None na koncu ogona.
        oldest = engine.retained_from("src")
        tail = [row[0] for row in engine.rows("src")]
        assert len(tail) == engine.record_count("src") - oldest
        assert all(value is None or value in INPUT for value in tail)
        if oldest:
            with pytest.raises(IndexError):
                engine.record("src", 0)


def test_recompile_replaces_the_plan(rdb, plan_dir: Path) -> None:
    other = write_source(plan_dir, [1, 2, 3], name="other.txt")
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        engine.compile(f"DECLARE a INTEGER STREAM src, 1 TEXTFILE '{other}'\nSELECT a*3 STREAM tripled FROM src\n")
        assert engine.slots_done == 0
        assert engine.streams() == ["src", "tripled"]
        engine.run()
        assert [row[0] for row in engine.rows("tripled")] == [3, 6, 9]


def test_recompile_in_the_same_directory_reads_the_new_source(rdb, plan_dir: Path) -> None:
    """A stale src.desc carries REF to the previous file; the plan must win over it.

    The service refuses a kept declaration .desc that disagrees with the plan (#265);
    the engine replaces the declaration's descriptor in its owned directory (#450),
    because re-running a cell with a new file is normal.
    """
    other = write_source(plan_dir, [1, 2, 3], name="other.txt")
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        engine.compile(doubling_plan(other))
        engine.run()
        assert engine.record_count("dst") == 3, "dst kept the previous run's records"
        assert [row[0] for row in engine.rows("dst")] == [2, 4, 6]


def test_wrapping_source_when_until_eof_is_off(rdb, plan_dir: Path) -> None:
    """The daemon's default: a source without ONESHOT wraps to its start after EOF."""
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"), until_eof=False)
        assert engine.run(slots=14) == 14
        assert not engine.end_of_input
        assert engine.record_count("dst") > len(INPUT)


def test_two_engines_do_not_share_a_volatile_stream(rdb, plan_dir: Path) -> None:
    """The phase-2 isolation claim, now for a RUNNING plan rather than a bare storage.

    Both engines compute a MEMORY-backed stream of the same name from different
    inputs. Before dataModel took a MemoryStore this would have read through one
    process-wide map and the second engine would have seen the first one's values.
    """
    low = write_source(plan_dir, [1, 2, 3, 4], name="low.txt")
    high = write_source(plan_dir, [100, 200, 300, 400], name="high.txt")

    def plan(source: Path) -> str:
        return f"DECLARE a INTEGER STREAM src, 1 TEXTFILE '{source}'\nSELECT a*2 STREAM shared FROM src VOLATILE\n"

    with rdb.Engine(str(plan_dir / "storage")) as first, rdb.Engine() as second:
        first.compile(plan(low))
        second.compile(plan(high))
        first.run()
        second.run()
        # VOLATILE jest pierscieniem MEMORY o rozmiarze z kompilatora (tu 1): rows() oddaje
        # tylko zachowany ogon, a record_count liczy wszystkie zapisy.
        assert first.record_count("shared") == second.record_count("shared") == 4
        assert first.retained_from("shared") == second.retained_from("shared") == 3
        assert [row[0] for row in first.rows("shared")] == [8]
        assert [row[0] for row in second.rows("shared")] == [800]
        with pytest.raises(IndexError):
            first.record("shared", 0)


def test_engine_logs_reach_the_logging_module(rdb, plan_dir: Path, caplog: pytest.LogCaptureFixture) -> None:
    # covers: module.python-logging-callback.1~1
    """Engine diagnostics go to logging.getLogger('retractordb'), never to stdout.

    A clean run logs nothing, so the record comes from a deterministic error path:
    qTree::getQuery logs "Missing - <name>" at ERROR before it throws.
    """
    caplog.set_level(logging.DEBUG, logger="retractordb")
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile("SELECT a*2 STREAM dst FROM nosuch\n")
    engine_records = [record for record in caplog.records if record.name == "retractordb"]
    assert engine_records, "no engine log record arrived"
    assert any("nosuch" in record.getMessage() for record in engine_records)
    assert all(record.levelno >= logging.DEBUG for record in engine_records)


def test_logging_handler_failure_preserves_the_plan_error_and_next_compile(rdb, plan_dir: Path, monkeypatch) -> None:
    # covers: module.python-logging-callback.2~1
    failures = []

    def record_failure(item):
        failures.append((type(item.exc_value), str(item.exc_value)))

    monkeypatch.setattr(sys, "unraisablehook", record_failure)

    class RejectLog(logging.Handler):
        def emit(self, record):
            raise RuntimeError("injected Python logging failure")

    logger = logging.getLogger("retractordb")
    handler = RejectLog()
    logger.addHandler(handler)
    try:
        with rdb.Engine() as engine:
            with pytest.raises(rdb.CompileError):
                engine.compile("SELECT a*2 STREAM dst FROM nosuch\n")
            assert not engine.failed
            engine.compile(doubling_plan(plan_dir / "data.txt"))
            assert engine.run() == len(INPUT)
    finally:
        logger.removeHandler(handler)
    assert failures
    assert all(kind is RuntimeError for kind, _ in failures)
    assert all("injected Python logging failure" in message for _, message in failures)


def test_run_honours_keyboard_interrupt(rdb, plan_dir: Path) -> None:
    """A wrapping plan runs forever; the stop button must still work.

    ``_thread.interrupt_main`` raises the same flag Ctrl-C does, and ``run()`` picks
    it up through PyErr_CheckSignals while the GIL is released for the slots.
    """
    import _thread

    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"), until_eof=False)
        timer = threading.Timer(0.2, _thread.interrupt_main)
        timer.start()
        try:
            with pytest.raises(KeyboardInterrupt):
                engine.run()
        finally:
            timer.cancel()
        assert engine.slots_done > 0
        # Silnik przezyl przerwanie i dalej liczy.
        assert engine.run(slots=2) == 2


def test_exception_hierarchy(rdb) -> None:
    """The hierarchy decided in #491: K2 and K3 are ValueError, K4 is OSError."""
    assert issubclass(rdb.ConfigError, rdb.RetractorDBError)
    assert issubclass(rdb.ConfigError, ValueError)
    assert issubclass(rdb.PlanError, rdb.ConfigError)
    assert issubclass(rdb.RQLSyntaxError, rdb.PlanError)
    assert issubclass(rdb.CompileError, rdb.PlanError)
    assert issubclass(rdb.IOError, rdb.RetractorDBError)
    assert issubclass(rdb.IOError, OSError)
    assert issubclass(rdb.InternalError, rdb.RetractorDBError)
    assert not issubclass(rdb.InternalError, (ValueError, OSError))
    assert not issubclass(rdb.IOError, ValueError)


def test_fault_in_a_slot_is_an_io_error_and_latches_the_plan(rdb, plan_dir: Path) -> None:
    """K4 inside a slot reaches Python as IOError and the kernel lives (decision (a)).

    The model may stand half-way through the slot, so the plan refuses to go on until it
    is compiled again; every later refusal keeps the first error's class and names its
    cause. Raised as InternalError, it would taint the module like a real K1 (#476).
    The fault hook is the one it_env_error_exit uses on the service.
    """
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        os.environ["RDB_FAULT_ENV_IN_SLOT"] = "1"
        try:
            with pytest.raises(rdb.IOError):
                engine.step()
        finally:
            del os.environ["RDB_FAULT_ENV_IN_SLOT"]
        assert engine.failed
        with pytest.raises(rdb.IOError, match="RDB_FAULT_ENV_IN_SLOT"):
            engine.step()
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        assert not engine.failed
        engine.run()
        assert [row[0] for row in engine.rows("dst")] == [value * 2 for value in INPUT]


def storage_bytes(directory: Path) -> dict[str, bytes]:
    return {str(path.relative_to(directory)): path.read_bytes() for path in directory.rglob("*") if path.is_file()}


def test_default_directories_are_private_and_removed_with_the_engine(rdb, plan_dir: Path) -> None:
    first, second = rdb.Engine(), rdb.Engine()
    first_dir, second_dir = Path(first.storage_dir), Path(second.storage_dir)
    assert first_dir != second_dir
    first.compile(doubling_plan(plan_dir / "data.txt"))
    second.compile(doubling_plan(plan_dir / "data.txt").replace("a*2", "a*3"))
    first.run()
    second.run()
    assert [row[0] for row in first.rows("dst")] == [value * 2 for value in INPUT]
    assert [row[0] for row in second.rows("dst")] == [value * 3 for value in INPUT]
    first.close()
    assert first_dir.is_dir(), "close() must allow recompile in the same directory"
    del first, second
    assert not first_dir.exists()
    assert not second_dir.exists()


@pytest.mark.parametrize("descriptor", ["{ a INTEGER[1] }", "{ a DOUBLE[1] }", "broken descriptor"])
@pytest.mark.parametrize("directive", [False, True], ids=["constructor", "STORAGE"])
def test_foreign_storage_is_refused_without_changing_any_files(rdb, plan_dir: Path, descriptor: str, directive: bool) -> None:
    directory = plan_dir / "storage"
    for name, value in {"src.desc": descriptor, "src.meta": "old metadata", "dst": "old data", "dst.desc": descriptor}.items():
        (directory / name).write_text(value)
    before = storage_bytes(directory)
    plan = doubling_plan(plan_dir / "data.txt")
    with rdb.Engine("" if directive else str(directory)) as engine:
        with pytest.raises(rdb.CompileError, match="empty directory"):
            engine.compile((f"STORAGE '{directory}'\n" if directive else "") + plan)
        assert not engine.has_plan
    assert storage_bytes(directory) == before
    with rdb.Engine() as usable:
        usable.compile(doubling_plan(plan_dir / "data.txt"))
        assert usable.run() == len(INPUT)


@pytest.mark.parametrize("alias", [False, True], ids=["same-path", "symlink-alias"])
def test_second_engine_is_refused_until_the_first_closes(rdb, plan_dir: Path, alias: bool) -> None:
    directory = plan_dir / "storage"
    second_dir = plan_dir / "alias" if alias else directory
    if alias:
        second_dir.symlink_to(directory, target_is_directory=True)
    plan = doubling_plan(plan_dir / "data.txt")
    with rdb.Engine(str(directory)) as first, rdb.Engine(str(second_dir)) as second:
        first.compile(plan)
        before = storage_bytes(directory)
        with pytest.raises(rdb.ConfigError, match="busy"):
            second.compile(plan)
        assert storage_bytes(directory) == before
        first.run()
        assert [row[0] for row in first.rows("dst")] == [value * 2 for value in INPUT]
        first.close()
        second.compile(plan)
        second.run()
        assert [row[0] for row in second.rows("dst")] == [value * 2 for value in INPUT]


def test_directory_lock_also_excludes_another_process(rdb, module_root: Path, plan_dir: Path) -> None:
    directory = plan_dir / "storage"
    plan = doubling_plan(plan_dir / "data.txt")
    program = f"""
import sys
sys.path.insert(0, {str(module_root)!r})
import retractordb as rdb
with rdb.Engine({str(directory)!r}) as engine:
    try:
        engine.compile({plan!r})
    except rdb.ConfigError as error:
        assert 'busy' in str(error)
        print('refused; kernel alive')
    else:
        raise AssertionError('another process acquired the directory')
"""
    with rdb.Engine(str(directory)) as first:
        first.compile(plan)
        result = subprocess.run([sys.executable, "-c", program], capture_output=True, text=True, timeout=15)
        assert result.returncode == 0, result.stderr
        assert "refused; kernel alive" in result.stdout
        assert first.run() == len(INPUT)


def test_process_death_releases_the_directory_lock(rdb, module_root: Path, plan_dir: Path) -> None:
    directory = plan_dir / "storage"
    plan = doubling_plan(plan_dir / "data.txt")
    program = f"""
import os, sys
sys.path.insert(0, {str(module_root)!r})
import retractordb as rdb
engine = rdb.Engine({str(directory)!r})
engine.compile({plan!r})
engine.run(slots=2)
os._exit(0)
"""
    result = subprocess.run([sys.executable, "-c", program], capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stderr
    with rdb.Engine(str(directory)) as engine:
        engine.compile(plan)
        engine.run()
        assert [row[0] for row in engine.rows("dst")] == [value * 2 for value in INPUT]


@pytest.mark.parametrize("output", ["missing/result", "../outside", ".retractordb-embedded", "link/outside"])
def test_bad_output_paths_preserve_the_previous_artifacts(rdb, plan_dir: Path, output: str) -> None:
    directory = plan_dir / "storage"
    outside = plan_dir / "outside"
    outside.write_bytes(b'outside data')
    plan = doubling_plan(plan_dir / "data.txt")
    with rdb.Engine(str(directory)) as engine:
        engine.compile(plan)
        engine.run()
        engine.close()
        (directory / "link").symlink_to(plan_dir, target_is_directory=True)
        before = storage_bytes(directory)
        with pytest.raises(rdb.CompileError):
            engine.compile(plan.replace("FROM src", f"FROM src FILE '{output}'"))
        assert not engine.has_plan
        assert storage_bytes(directory) == before
        assert outside.read_bytes() == b'outside data'
        assert (directory / "link").is_symlink()
        engine.compile(plan)
        assert engine.run() == len(INPUT)


def test_schema_can_change_in_an_owned_directory(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "storage")) as engine:
        plan = doubling_plan(plan_dir / "data.txt")
        engine.compile(plan)
        engine.run()
        engine.compile(plan.replace("a INTEGER", "a DOUBLE"))
        engine.run()
        assert engine.schema("src")[0].type == rdb.FieldType.DOUBLE
        assert [row[0] for row in engine.rows("dst")] == [float(value * 2) for value in INPUT]


def test_invalid_owner_marker_is_refused(rdb, plan_dir: Path) -> None:
    directory = plan_dir / "storage"
    (directory / ".retractordb-embedded").write_text("another format")
    before = storage_bytes(directory)
    with rdb.Engine(str(directory)) as engine:
        with pytest.raises(rdb.CompileError, match="owner marker"):
            engine.compile(doubling_plan(plan_dir / "data.txt"))
    assert storage_bytes(directory) == before


def test_source_in_storage_preserves_the_previous_artifacts(rdb, plan_dir: Path) -> None:
    directory = plan_dir / "storage"
    with rdb.Engine(str(directory)) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        engine.close()
        source = directory / "input.txt"
        source.write_text("1\n2\n")
        before = storage_bytes(directory)
        with pytest.raises(rdb.CompileError):
            engine.compile(doubling_plan(source))
        assert not engine.has_plan
        assert storage_bytes(directory) == before


def test_missing_source_is_a_typed_error_unlike_the_service(rdb, plan_dir: Path) -> None:
    # Serwis daje przy braku pliku zrodla rekordy NULL (#346); granica osadzona odmawia (#451).
    source = plan_dir / "input.txt"
    source.write_text("1\n2\n")
    with rdb.Engine() as engine:
        engine.compile(doubling_plan(source))
        for missing in (plan_dir / "missing.txt", plan_dir / "no_dir" / "missing.txt"):
            with pytest.raises(rdb.IOError, match="source file does not exist") as refused:
                engine.compile(doubling_plan(missing), until_eof=False)
            assert refused.value.errno == errno.ENOENT
            assert str(missing) in str(refused.value)
            assert not engine.has_plan
        engine.compile(doubling_plan(source))
        engine.run()
        assert [row[0] for row in engine.rows("dst")] == [2, 4]


def test_explicit_disposable_still_removes_the_external_source(rdb, plan_dir: Path) -> None:
    source = plan_dir / "data.txt"
    with rdb.Engine() as engine:
        engine.compile(doubling_plan(source).replace("'\nSELECT", "' DISPOSABLE\nSELECT"))
        engine.run()
        assert source.exists()
        engine.close()
        assert not source.exists()
