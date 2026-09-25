"""The embedded engine end to end: compile, step, read (J1 over core phase 3).

The plans mirror test/IntegrationTest/untileof_stop: eight integers in a text
source, one SELECT doubling them. What the daemon writes for ``xretractor -u``
is what ``Engine.run()`` must leave in the storage directory, record for record.
"""

from __future__ import annotations

import logging
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
    # Sciezka bezwzgledna: FILE jest wzgledne do katalogu roboczego PROCESU, a ten w
    # pytest nie jest katalogiem tymczasowym testu.
    return f"DECLARE a INTEGER STREAM src, 1/2 FILE '{source}'\nSELECT a*2 STREAM dst FROM src\n"


@pytest.fixture
def plan_dir(tmp_path: Path) -> Path:
    write_source(tmp_path)
    return tmp_path


def test_run_to_end_of_input_doubles_every_record(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir)) as engine:
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
    engine = rdb.Engine(str(plan_dir))
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
    with rdb.Engine(str(plan_dir)) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        assert engine.run(slots=3) == 3
        assert engine.slots_done == 3
        assert not engine.end_of_input
        assert engine.run(slots=2) == 2
        assert engine.slots_done == 5


def test_artefacts_land_in_the_engine_storage_dir(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir)) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
    assert (plan_dir / "dst").exists()
    assert (plan_dir / "dst.desc").exists()
    # Magazyn jest zwykly: czyta go tez wiazanie etapu 1a.
    with rdb.Storage("dst", "dst", storage_param=str(plan_dir)) as storage:
        assert [record[0] for record in storage] == [value * 2 for value in INPUT]


def test_storage_directive_in_the_plan_wins(rdb, plan_dir: Path) -> None:
    inside = plan_dir / "inside"
    inside.mkdir()
    plan = f"STORAGE '{inside}'\n" + doubling_plan(plan_dir / "data.txt")
    with rdb.Engine(str(plan_dir)) as engine:
        engine.compile(plan)
        engine.run()
    assert (inside / "dst").exists()
    assert not (plan_dir / "dst").exists()


def test_missing_storage_dir_is_a_config_error_not_a_dead_kernel(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir / "nowhere")) as engine:
        with pytest.raises(rdb.ConfigError):
            engine.compile(doubling_plan(plan_dir / "data.txt"))
        assert not engine.has_plan


def test_syntax_error_names_the_problem(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir)) as engine:
        with pytest.raises(rdb.RQLSyntaxError) as failure:
            engine.compile("SELEKT nonsense FROM nowhere\n")
        assert issubclass(rdb.RQLSyntaxError, rdb.ConfigError)
        assert issubclass(rdb.RQLSyntaxError, rdb.RetractorDBError)
        assert str(failure.value)  # tresc parsera, nie pusty napis
        assert not engine.has_plan


def test_compile_error_for_an_unknown_source(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir)) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile("SELECT a*2 STREAM dst FROM nosuch\n")
        assert issubclass(rdb.CompileError, rdb.ConfigError)


def test_empty_plan_is_a_compile_error(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir)) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile("# only a comment\n")


@pytest.mark.parametrize(
    "extra",
    [
        "RULE r ON dst WHEN dst[0] > 100 DO DUMP -1 TO 1\n",
        "RULE r ON dst WHEN dst[0] > 100 DO SYSTEM 'echo no'\n",
        "ROTATION 'counter.txt'\n",
    ],
    ids=["dump-rule", "system-rule", "rotation"],
)
def test_daemon_only_features_are_refused_at_compile(rdb, plan_dir: Path, extra: str) -> None:
    """DUMP needs the daemon's global model pointer, SYSTEM waits for a host callback,
    ROTATION reads the daemon's counter. Refused up front, not half-way through a slot."""
    with rdb.Engine(str(plan_dir)) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile(doubling_plan(plan_dir / "data.txt") + extra)
        assert not engine.has_plan


def test_operations_without_a_plan_raise_config_error(rdb) -> None:
    engine = rdb.Engine()
    with pytest.raises(rdb.ConfigError):
        engine.step()
    with pytest.raises(rdb.ConfigError):
        engine.streams()
    assert engine.slots_done == 0
    assert engine.time == 0


def test_record_access_follows_python_conventions(rdb, plan_dir: Path) -> None:
    with rdb.Engine(str(plan_dir)) as engine:
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
    with rdb.Engine(str(plan_dir)) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        engine.compile(f"DECLARE a INTEGER STREAM src, 1 FILE '{other}'\nSELECT a*3 STREAM tripled FROM src\n")
        assert engine.slots_done == 0
        assert engine.streams() == ["src", "tripled"]
        engine.run()
        assert [row[0] for row in engine.rows("tripled")] == [3, 6, 9]


def test_recompile_in_the_same_directory_reads_the_new_source(rdb, plan_dir: Path) -> None:
    """A stale src.desc carries REF to the previous file; the plan must win over it.

    storage::attachDescriptor prefers an existing .desc and checks only the data
    fields, so without dropping the declaration's descriptor the second plan would
    quietly keep reading data.txt. Re-running a cell with a new file is normal.
    """
    other = write_source(plan_dir, [1, 2, 3], name="other.txt")
    with rdb.Engine(str(plan_dir)) as engine:
        engine.compile(doubling_plan(plan_dir / "data.txt"))
        engine.run()
        engine.compile(doubling_plan(other))
        engine.run()
        assert engine.record_count("dst") == 3, "dst kept the previous run's records"
        assert [row[0] for row in engine.rows("dst")] == [2, 4, 6]


def test_wrapping_source_when_until_eof_is_off(rdb, plan_dir: Path) -> None:
    """The daemon's default: a source without ONESHOT wraps to its start after EOF."""
    with rdb.Engine(str(plan_dir)) as engine:
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
        return f"DECLARE a INTEGER STREAM src, 1 FILE '{source}'\nSELECT a*2 STREAM shared FROM src VOLATILE\n"

    with rdb.Engine(str(plan_dir)) as first, rdb.Engine(str(plan_dir)) as second:
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
    """Engine diagnostics go to logging.getLogger('retractordb'), never to stdout.

    A clean run logs nothing, so the record comes from a deterministic error path:
    qTree::getQuery logs "Missing - <name>" at ERROR before it throws.
    """
    caplog.set_level(logging.DEBUG, logger="retractordb")
    with rdb.Engine(str(plan_dir)) as engine:
        with pytest.raises(rdb.CompileError):
            engine.compile("SELECT a*2 STREAM dst FROM nosuch\n")
    engine_records = [record for record in caplog.records if record.name == "retractordb"]
    assert engine_records, "no engine log record arrived"
    assert any("nosuch" in record.getMessage() for record in engine_records)
    assert all(record.levelno >= logging.DEBUG for record in engine_records)


def test_run_honours_keyboard_interrupt(rdb, plan_dir: Path) -> None:
    """A wrapping plan runs forever; the stop button must still work.

    ``_thread.interrupt_main`` raises the same flag Ctrl-C does, and ``run()`` picks
    it up through PyErr_CheckSignals while the GIL is released for the slots.
    """
    import _thread

    with rdb.Engine(str(plan_dir)) as engine:
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
