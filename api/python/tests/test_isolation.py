"""Embedded lifecycle and bus isolation, including abrupt process death (#451)."""

from __future__ import annotations

import gc
import hashlib
import os
import select
import subprocess
import sys
import time
from pathlib import Path

import pytest

from test_engine import doubling_plan, write_source


def snapshot(locks: Path, runtime: Path) -> dict:
    paths = list(Path("/dev/shm").iterdir())
    paths += list(Path("/tmp").glob("xrdbbus*.lock"))
    paths += list(Path("/tmp").glob("xretractor_ipc.*.lock"))
    paths += list(locks.glob("*.lock")) + list(runtime.glob("xretractor_service*.lock"))
    result = {}
    for path in paths:
        info = path.lstat()
        # Spis obejmuje cale /dev/shm; tresc obcych obiektow (np. IDE) moze zmieniac inny proces.
        retractor_object = path.parent != Path("/dev/shm") or path.name.startswith(("xrdb", "Retractor", "xretractor"))
        try:
            digest = hashlib.sha256(path.read_bytes()).hexdigest() if retractor_object and path.is_file() else None
        except PermissionError:
            digest = "unreadable"
        result[str(path)] = (info.st_ino, info.st_mode, info.st_uid, info.st_gid, info.st_size, digest)
    return result


def phase(child: subprocess.Popen, expected: str) -> None:
    ready, _, _ = select.select([child.stdout], [], [], 30)
    assert ready, f"embedded subprocess did not reach {expected}"
    assert child.stdout.readline().strip() == expected


def exercise_child(module_root: Path, body: str, environment: dict, locks: Path, runtime: Path, kill: bool) -> None:
    before = snapshot(locks, runtime)
    program = (f"import sys; sys.path.insert(0, {str(module_root)!r})\nimport retractordb as rdb\n"
               "engine = rdb.Engine()\n"
               f"engine.compile({body!r})\nprint('compiled', flush=True)\ninput()\n"
               "engine.run()\nprint('ran', flush=True)\ninput()\n"
               "engine.close()\nprint('closed', flush=True)\ninput()\n")
    with (runtime / "embedded-stderr.log").open("w+") as errors:
        child = subprocess.Popen([sys.executable, "-c", program], env=environment, stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE, stderr=errors, text=True)
        try:
            phase(child, "compiled")
            assert snapshot(locks, runtime) == before, "compile changed bus/presence state"
            child.stdin.write("\n")
            child.stdin.flush()
            phase(child, "ran")
            assert snapshot(locks, runtime) == before, "run changed bus/presence state"
            if kill:
                child.kill()
                assert child.wait(timeout=10) == -9
            else:
                child.stdin.write("\n")
                child.stdin.flush()
                phase(child, "closed")
                assert snapshot(locks, runtime) == before, "close changed bus/presence state"
                child.stdin.write("\n")
                child.stdin.flush()
                assert child.wait(timeout=10) == 0
            assert snapshot(locks, runtime) == before, "process exit changed bus/presence state"
        finally:
            if child.poll() is None:
                child.kill()
                child.wait(timeout=10)
            child.stdin.close()
            child.stdout.close()
            errors.seek(0)
            diagnostic = errors.read()
            assert child.returncode in (0, -9), diagnostic


@pytest.mark.parametrize("mode", ["alone", "standalone", "service"])
@pytest.mark.parametrize("kill", [False, True], ids=["close", "sigkill"])
def test_engine_does_not_touch_bus_or_reference(rdb, module_root, xretractor, tmp_path, mode, kill) -> None:
    if not Path("/dev/shm").is_dir():
        pytest.skip("bus snapshot acceptance requires POSIX /dev/shm")
    locks, runtime, storage = (tmp_path / name for name in ("locks", "runtime", "reference"))
    for directory in (locks, runtime, storage):
        directory.mkdir()
    source = write_source(tmp_path, [1, 2, 3, 4])
    body = doubling_plan(source)
    namespace = f"py451iso{os.getpid()}"
    environment = dict(os.environ, RDB_NAMESPACE=namespace, TMPDIR=str(runtime), XDG_CONFIG_HOME=str(tmp_path / "config"))
    if mode == "alone":
        exercise_child(module_root, body, environment, locks, runtime, kill)
        return
    config = tmp_path / "retractor.toml"
    config.write_text(f'[paths]\nlock_dir = "{locks}"\n')
    query = tmp_path / "query.rql"
    query.write_text(f"STORAGE '{storage}'\n" + body)
    command = [str(xretractor), "--config", str(config), "--name", namespace]
    xqry = xretractor.parent.parent / "qry/xqry"
    assert xqry.is_file(), f"acceptance requires rebuilt xqry: {xqry}"
    with (runtime / "reference.log").open("w+") as log:
        reference = subprocess.Popen(command + [str(query), "-u", "--no-clock", "--noanykey", "-x"] +
                                     (["--service"] if mode == "service" else []), env=environment, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 30
            while not (storage / "dst.meta").exists():
                assert reference.poll() is None, (runtime / "reference.log").read_text()
                assert time.monotonic() < deadline, "reference did not initialize"
                time.sleep(0.01)
            assert (storage / "dst").stat().st_size == 0
            assert any(namespace in path.name for path in Path("/dev/shm").iterdir())
            exercise_child(module_root, body, environment, locks, runtime, kill)
            assert reference.poll() is None
            assert (storage / "dst").stat().st_size == 0, "Engine lifted the reference gate"
            # Dopiero jawne polecenie do referencji otwiera bramke; nie wysylamy go przed snapshotami.
            hello = subprocess.run([str(xqry), "--config", str(config), "--server", namespace, "--hello"],
                                   env=environment, capture_output=True, text=True, timeout=20)
            assert hello.returncode == 0, hello.stderr
            assert reference.wait(timeout=20) == 0, (runtime / "reference.log").read_text()
        finally:
            if reference.poll() is None:
                reference.kill()
                reference.wait(timeout=10)
            cleanup = subprocess.run(command + ["--cleanup"], env=environment, capture_output=True, text=True, timeout=10)
            assert cleanup.returncode == 0, cleanup.stderr
    with rdb.Storage("dst", "dst", storage_param=str(storage), lock_dir=str(locks)) as stored:
        assert [tuple(stored.read(index)) for index in range(stored.record_count)] == [(2,), (4,), (6,), (8,)]


def test_engine_context_and_destructor_release_owned_storage(rdb, tmp_path) -> None:
    source = write_source(tmp_path)
    directory = tmp_path / "owned"
    directory.mkdir()
    with pytest.raises(RuntimeError, match="cell failed"):
        with rdb.Engine(str(directory), lock_dir=str(tmp_path)) as engine:
            engine.compile(doubling_plan(source))
            raise RuntimeError("cell failed")
    assert not engine.has_plan
    engine.close()
    with rdb.Engine(str(directory), lock_dir=str(tmp_path)) as replacement:
        replacement.compile(doubling_plan(source))
    del engine, replacement
    private = rdb.Engine()
    private.compile(doubling_plan(source))
    path = Path(private.storage_dir)
    del private
    gc.collect()
    assert not path.exists()

