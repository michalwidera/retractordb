"""Reading records through the embedded storage binding."""

from __future__ import annotations

import errno
import fcntl
import os
import subprocess
import time
from pathlib import Path

import pytest


def open_plain(rdb, directory: Path):
    return rdb.Storage("plain_file", "plain_file", storage_param=str(directory))


def test_storage_reports_its_shape(rdb, plain_storage: Path) -> None:
    with open_plain(rdb, plain_storage) as storage:
        assert storage.record_count == 4
        assert len(storage) == 4
        assert not storage.is_declared
        assert [field.name for field in storage.descriptor] == ["a", "b"]


def test_reads_the_recorded_values(rdb, plain_storage: Path) -> None:
    """Values come from generate_storage_map_fixtures.py: (1,10) (2,20) (3,30) (4,40)."""
    with open_plain(rdb, plain_storage) as storage:
        first = storage.read(0)
        assert len(first) == 2
        assert (first[0], first[1]) == (1, 10)
        assert first.index == 0

        assert tuple(storage.read(1)) == (2, 20)


def test_null_metadata_reaches_python_as_none(rdb, plain_storage: Path) -> None:
    """The meta index marks record 2 partially null and record 3 entirely null.

    Source: the segment layout written by generate_storage_map_fixtures.py and
    rendered in out-plain.local.txt as [====]2 [----]1 [~~~~]1 [XXXX]gap.
    """
    with open_plain(rdb, plain_storage) as storage:
        assert storage.read(2)[0] is None
        assert tuple(storage.read(3)) == (None, None)


def test_negative_index_reads_from_the_end(rdb, plain_storage: Path) -> None:
    """A negative index is how revRead() is spelled in Python."""
    with open_plain(rdb, plain_storage) as storage:
        assert tuple(storage[-4]) == tuple(storage[0])
        assert storage[-1].index == 3


def test_out_of_range_raises_rather_than_returning_zeros(rdb, plain_storage: Path) -> None:
    """storage::read() logs and hands back a zeroed record for a bad index.

    A silently wrong value is worse than an exception, so the binding checks the
    range itself. This test pins that guard - without it the call below would
    quietly succeed and return zeros.
    """
    with open_plain(rdb, plain_storage) as storage:
        with pytest.raises(IndexError):
            storage.read(4)
        with pytest.raises(IndexError):
            storage.read(-5)


def test_a_record_survives_the_next_read(rdb, plain_storage: Path) -> None:
    """Record holds its own payload copy, so it does not alias the storage buffer."""
    with open_plain(rdb, plain_storage) as storage:
        kept = storage.read(0)
        storage.read(1)
        assert tuple(kept) == (1, 10)


def test_a_shadow_update_wins_over_the_data_file(rdb, shadowed_storage: Path) -> None:
    """The read path consults the shadow first, and that is visible from Python.

    posixBinaryFileWithShadow::read() calls shadowFind() before touching the data
    file (faccposixshd.cc), and this fixture's shadow carries one entry
    for byte offset 0 holding (0,0). So record 0 reads as the pending update
    while records 1-3 fall through to the data file.

    Pinned here because it is not obvious from the API and because it is the
    reason `plain_storage` omits the shadow - see the note in conftest.py.
    """
    with open_plain(rdb, shadowed_storage) as storage:
        assert tuple(storage.read(0)) == (0, 0)
        assert tuple(storage.read(1)) == (2, 20)


def test_missing_storage_directory_raises(rdb, tmp_path: Path) -> None:
    with pytest.raises(rdb.NoSuchStream):
        rdb.Storage("plain_file", "plain_file", storage_param=str(tmp_path / "absent"))


def test_missing_descriptor_raises(rdb, tmp_path: Path) -> None:
    with pytest.raises(rdb.NoSuchStream):
        rdb.Storage("no_such_stream", "no_such_stream", storage_param=str(tmp_path))


def test_empty_identifiers_are_rejected(rdb, plain_storage: Path) -> None:
    """StoragePaths reports an empty id as K1 (FatalError); the binding refuses first with ValueError."""
    with pytest.raises(ValueError):
        rdb.Storage("", "plain_file", storage_param=str(plain_storage))
    with pytest.raises(ValueError):
        rdb.Storage("plain_file", "", storage_param=str(plain_storage))


def test_errors_share_one_base(rdb) -> None:
    assert issubclass(rdb.NoSuchStream, rdb.RetractorDBError)
    assert issubclass(rdb.StorageError, rdb.RetractorDBError)


def test_context_exit_really_closes_the_storage(rdb, plain_storage: Path) -> None:
    metadata = (plain_storage / "plain_file.meta").read_bytes()
    with open_plain(rdb, plain_storage) as storage:
        kept = storage.read(0)
        assert not storage.closed
    assert storage.closed
    assert (plain_storage / "plain_file.meta").read_bytes() == metadata
    storage.close()
    for operation in (lambda: storage.read(0), lambda: len(storage), lambda: storage.descriptor,
                      lambda: storage.record_count, lambda: storage.is_declared, lambda: storage.__enter__()):
        with pytest.raises(rdb.StorageError, match="closed"):
            operation()
    assert tuple(kept) == (1, 10)


def test_context_exit_closes_on_exception(rdb, plain_storage: Path) -> None:
    with pytest.raises(RuntimeError, match="body failed"):
        with open_plain(rdb, plain_storage) as storage:
            storage.read(0)
            raise RuntimeError("body failed")
    assert storage.closed


def test_unmarked_offline_storage_still_allows_independent_readers(rdb, plain_storage: Path) -> None:
    with open_plain(rdb, plain_storage) as first, open_plain(rdb, plain_storage) as second:
        assert tuple(first[0]) == tuple(second[0]) == (1, 10)


def test_close_is_idempotent_and_record_copy_survives(rdb, plain_storage: Path) -> None:
    storage = open_plain(rdb, plain_storage)
    record = storage[0]
    storage.close()
    storage.close()
    assert storage.closed
    assert tuple(record) == (1, 10)
    with pytest.raises(rdb.StorageError, match="closed"):
        storage[0]


def test_engine_and_storage_share_the_owned_directory_lock(rdb, tmp_path: Path) -> None:
    from test_engine import doubling_plan

    source = tmp_path / "source.txt"
    source.write_text("1\n2\n3\n")
    with rdb.Engine() as engine:
        directory = engine.storage_dir
        plan = doubling_plan(source)
        engine.compile(plan)
        engine.run()
        with pytest.raises(rdb.ConfigError, match="busy"):
            rdb.Storage("dst", "dst", storage_param=directory)
        engine.close()
        with rdb.Storage("dst", "dst", storage_param=directory) as storage:
            assert [row[0] for row in storage] == [2, 4, 6]
            with pytest.raises(rdb.ConfigError, match="busy"):
                engine.compile(plan)
            with pytest.raises(rdb.ConfigError, match="busy"):
                rdb.Storage("dst", "dst", storage_param=directory)
        assert storage.closed
        engine.compile(plan)
        assert engine.run() == 3


@pytest.mark.parametrize("default_directory", [False, True], ids=["explicit-lock-dir", "TMPDIR"])
def test_held_service_lock_refuses_storage_and_engine(rdb, plain_storage: Path, tmp_path: Path,
                                                     monkeypatch, default_directory: bool) -> None:
    from test_engine import doubling_plan, storage_bytes

    locks = tmp_path / "locks"
    locks.mkdir()
    if default_directory:
        monkeypatch.setenv("TMPDIR", str(locks))
    lock_dir = "" if default_directory else str(locks)
    lock = locks / "xretractor_service.named.lock"
    owned = tmp_path / "owned"
    owned.mkdir()
    source = tmp_path / "source.txt"
    source.write_text("1\n2\n3\n")
    with lock.open("w") as held:
        held.write("PID: test holder\n")
        held.flush()
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        before = storage_bytes(plain_storage)
        # Trzymany flock to odmowa w trybie scislym i lagodnym.
        with pytest.raises(rdb.ConfigError, match="busy"):
            rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=lock_dir)
        with rdb.Engine(str(owned), lock_dir=lock_dir) as engine:
            with pytest.raises(rdb.ConfigError, match="busy"):
                engine.compile(doubling_plan(source))
        assert storage_bytes(plain_storage) == before
        assert not any(owned.iterdir())
        # Katalog prywatny z mkdtemp nie moze byc katalogiem serwisu: kontrola go nie dotyczy.
        with rdb.Engine(lock_dir=lock_dir) as engine:
            engine.compile(doubling_plan(source))
            assert engine.run() == 3
        assert lock.read_text() == "PID: test holder\n"
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=lock_dir) as storage:
        assert tuple(storage[0]) == (1, 10)


def write_config(path: Path, lock_dir: Path | None = None, lenient: list[str] | None = None) -> Path:
    text = f'[paths]\nlock_dir = "{lock_dir}"\n' if lock_dir else ""
    if lenient is not None:
        text += "[embedded]\nlenient_lock_dirs = [" + ", ".join(f'"{item}"' for item in lenient) + "]\n"
    path.write_text(text)
    return path


def plant_unreadable_lock(locks: Path) -> None:
    # Obcy wpis z nazwa z rodziny blokad, jak symlink podrzucony we wspoldzielonym /tmp.
    (locks / "xretractor_service.planted.lock").symlink_to(locks / "elsewhere")


def test_default_lock_check_is_lenient_like_xtrdb(rdb, plain_storage: Path, tmp_path: Path, monkeypatch) -> None:
    locks = tmp_path / "locks"
    locks.mkdir()
    plant_unreadable_lock(locks)
    (locks / "xretractor_service.dir.lock").mkdir()
    (locks / "xretractor_service.stale.lock").write_text("PID: gone\n")
    monkeypatch.setenv("TMPDIR", str(locks))
    with open_plain(rdb, plain_storage) as storage:
        assert tuple(storage[0]) == (1, 10)
    monkeypatch.setenv("TMPDIR", str(tmp_path / "absent"))
    with open_plain(rdb, plain_storage) as storage:
        assert tuple(storage[0]) == (1, 10)


def test_explicit_lock_dir_is_strict_unless_listed_as_lenient(rdb, plain_storage: Path, tmp_path: Path) -> None:
    locks = tmp_path / "locks"
    locks.mkdir()
    plant_unreadable_lock(locks)
    with pytest.raises(rdb.IOError) as failure:
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(locks))
    assert failure.value.errno == errno.ELOOP
    # Ten sam katalog, takze podany aliasem, wymieniony w embedded.lenient_lock_dirs.
    config = write_config(tmp_path / "lenient.toml", lenient=[str(tmp_path / "." / "locks"), str(tmp_path / "absent")])
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(locks),
                     config=str(config)) as storage:
        assert tuple(storage[0]) == (1, 10)
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(tmp_path / "absent"),
                     config=str(config)) as storage:
        assert tuple(storage[0]) == (1, 10)


def test_system_temp_is_lenient_until_the_key_says_otherwise(rdb, plain_storage: Path, tmp_path: Path,
                                                             monkeypatch) -> None:
    locks = tmp_path / "locks"
    locks.mkdir()
    plant_unreadable_lock(locks)
    monkeypatch.setenv("TMPDIR", str(locks))
    # Bez klucza lenient_lock_dirs lagodny jest z definicji katalog tymczasowy systemu, takze jawnie podany.
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(locks)) as storage:
        assert tuple(storage[0]) == (1, 10)
    # Podany klucz zastepuje te definicje: pusta lista czyni scislym takze domyslny TMPDIR.
    config = write_config(tmp_path / "strict.toml", lenient=[])
    for lock_dir in ("", str(locks)):
        with pytest.raises(rdb.IOError) as failure:
            rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=lock_dir,
                        config=str(config))
        assert failure.value.errno == errno.ELOOP


def test_guessed_lock_directory_is_reported(rdb, plain_storage: Path, tmp_path: Path, caplog) -> None:
    import logging
    import warnings

    locks = tmp_path / "locks"
    locks.mkdir()
    with caplog.at_level(logging.INFO, logger="retractordb"):
        with pytest.warns(UserWarning, match="TMPDIR as a guess"):
            open_plain(rdb, plain_storage).close()
    assert [(record.levelno, "TMPDIR as a guess" in record.getMessage()) for record in caplog.records
            if record.name == "retractordb"] == [(logging.INFO, True)]
    # Plik bez paths.lock_dir nadal zgaduje; jawny katalog z argumentu albo z pliku nie.
    with pytest.warns(UserWarning, match="TMPDIR as a guess"):
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage),
                    config=str(write_config(tmp_path / "empty.toml"))).close()
    with warnings.catch_warnings():
        warnings.simplefilter("error")
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(locks)).close()
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage),
                    config=str(write_config(tmp_path / "service.toml", lock_dir=locks))).close()
        with rdb.Engine() as engine:
            assert engine.storage_dir
    # Ostrzezenie zamienione w blad nie zostawia otwartego magazynu ani skazonego modulu.
    with warnings.catch_warnings():
        warnings.simplefilter("error")
        with pytest.raises(UserWarning):
            open_plain(rdb, plain_storage)
    with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(locks)) as storage:
        assert tuple(storage[0]) == (1, 10)


def test_config_supplies_the_lock_dir_and_the_argument_wins(rdb, plain_storage: Path, tmp_path: Path) -> None:
    locks, other = tmp_path / "locks", tmp_path / "other"
    locks.mkdir()
    other.mkdir()
    config = write_config(tmp_path / "service.toml", lock_dir=locks)
    with (locks / "xretractor_service.named.lock").open("w") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        with pytest.raises(rdb.ConfigError, match="busy"):
            rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), config=str(config))
        with rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(other),
                         config=str(config)) as storage:
            assert tuple(storage[0]) == (1, 10)


@pytest.mark.parametrize("problem", ["missing", "relative-lenient", "relative-lock-dir", "syntax"])
def test_bad_config_is_a_config_error_and_keeps_the_kernel(rdb, plain_storage: Path, tmp_path: Path,
                                                         problem: str) -> None:
    config = tmp_path / "bad.toml"
    if problem == "relative-lenient":
        write_config(config, lenient=["relative/locks"])
    elif problem == "relative-lock-dir":
        config.write_text('[paths]\nlock_dir = "relative/locks"\n')
    elif problem == "syntax":
        config.write_text("[paths\n")
    with pytest.raises(rdb.ConfigError, match="bad.toml|lenient_lock_dirs"):
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), config=str(config))
    with pytest.raises(rdb.ConfigError, match="bad.toml|lenient_lock_dirs"):
        rdb.Engine(config=str(config))
    with open_plain(rdb, plain_storage) as storage:
        assert tuple(storage[0]) == (1, 10)


def test_missing_lock_directory_preserves_errno_and_kernel(rdb, plain_storage: Path, tmp_path: Path) -> None:
    with pytest.raises(rdb.IOError) as failure:
        rdb.Storage("plain_file", "plain_file", storage_param=str(plain_storage), lock_dir=str(tmp_path / "absent"))
    assert failure.value.errno == errno.ENOENT
    with open_plain(rdb, plain_storage) as storage:
        assert tuple(storage[0]) == (1, 10)


def test_real_daemon_refuses_offline_access_without_changing_its_artifacts(rdb, module_root: Path, tmp_path: Path) -> None:
    from test_engine import doubling_plan, storage_bytes

    executable = Path(os.environ.get("RDB_XRETRACTOR", module_root.parent / "src/retractor/xretractor"))
    if not executable.is_file():
        pytest.skip("live daemon test requires a built xretractor or RDB_XRETRACTOR")
    locks, runtime, directory = (tmp_path / name for name in ("locks", "runtime", "storage"))
    for path in (locks, runtime, directory):
        path.mkdir()
    source = tmp_path / "source.txt"
    source.write_text("1\n2\n3\n")
    config = tmp_path / "retractor.toml"
    config.write_text(f'[paths]\nlock_dir = "{locks}"\n')
    query = tmp_path / "query.rql"
    query.write_text(f"STORAGE '{directory}'\n" + doubling_plan(source))
    namespace = f"py450{os.getpid()}"
    environment = dict(os.environ, TMPDIR=str(runtime), XDG_CONFIG_HOME=str(tmp_path / "config"), RDB_NAMESPACE=namespace)
    command = [str(executable), "--config", str(config), "--name", namespace]
    with (tmp_path / "daemon.log").open("w+") as log:
        daemon = subprocess.Popen(command + ["--service", "--noanykey", "-x", str(query)],
                                  env=environment, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 20
            while not all((directory / name).exists() for name in ("dst.desc", "dst", "dst.meta", "dst.shadow")):
                assert daemon.poll() is None, (tmp_path / "daemon.log").read_text()
                assert time.monotonic() < deadline, "daemon did not initialize its storage"
                time.sleep(0.01)
            before = storage_bytes(directory)
            with pytest.raises(rdb.ConfigError, match="busy"):
                rdb.Storage("dst", "dst", storage_param=str(directory), config=str(config))
            with rdb.Engine(str(directory), config=str(config)) as engine:
                with pytest.raises(rdb.ConfigError, match="busy"):
                    engine.compile(doubling_plan(source))
                assert not engine.has_plan
            assert storage_bytes(directory) == before
            assert daemon.poll() is None
        finally:
            daemon.terminate()
            try:
                daemon.wait(timeout=10)
            except subprocess.TimeoutExpired:
                daemon.kill()
                daemon.wait(timeout=5)
            subprocess.run(command + ["--cleanup"], env=environment, check=True, timeout=10, capture_output=True)
        assert daemon.returncode == 0, (tmp_path / "daemon.log").read_text()
    with rdb.Storage("dst", "dst", storage_param=str(directory), lock_dir=str(locks)) as storage:
        assert len(storage) == 0, "a daemon stopped at the gate must not have processed records"
    with rdb.Engine(lock_dir=str(locks)) as usable:
        usable.compile(doubling_plan(source))
        assert usable.run() == 3
