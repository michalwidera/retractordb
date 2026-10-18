"""#488: niedostepne zrodlo to K4 przed przetwarzaniem, a nie EOF ani dane NULL."""

import errno
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time


ENGINE = str(Path(sys.argv[1]).resolve())
LOG = Path(os.environ["TMPDIR"]) / "xretractor.log"


def invoke(directory, *options):
    runtime = [] if "-c" in options else ["-k", "-f"]
    return subprocess.run(
        [ENGINE, "plan.rql", *runtime, *options], cwd=directory,
        capture_output=True, text=True, timeout=5,
    )


def snapshot(directory):
    return {path.name: path.read_bytes() for path in (directory / "store").iterdir()}


def prepare(directory, kind, source, target=None):
    (directory / "store").mkdir()
    (directory / "plan.rql").write_text(
        f"STORAGE 'store'\nDECLARE a INTEGER STREAM src, 1 {kind} '{source}'\n"
        "SELECT a STREAM result FROM src\n", encoding="ascii",
    )
    path = directory / source
    path.parent.mkdir(parents=True, exist_ok=True)
    if target is not None:
        path.symlink_to(target)
        path = directory / target
    if kind == "DEVICE":
        os.mkfifo(path)
        writer = os.open(path, os.O_RDWR | os.O_NONBLOCK)
        os.write(writer, struct.pack("=2i", 7, 8))
        return writer
    path.write_bytes(b"7\n8\n" if kind == "TEXTFILE" else struct.pack("=2i", 7, 8))
    return None


def check_refusal(result, source, error):
    assert result.returncode == errno.EIO, (result.returncode, result.stdout, result.stderr)
    for text in ("stream 'src'", str(source), os.strerror(error)):
        assert text in result.stderr, (text, result.stderr)
    assert "FATAL" not in result.stderr, result.stderr


def missing_before_start(directory, kind, fault):
    directory.mkdir()
    source = Path("parent/source") if fault in ("missing_parent", "not_directory") else Path("source")
    target = "target" if fault == "dangling" else None
    writer = prepare(directory, kind, source, target)
    try:
        valid = invoke(directory, "-m", "3")
        assert valid.returncode == 0, valid.stderr
        assert (directory / "store/result").read_bytes() == struct.pack("=2i", 7, 8)
    finally:
        if writer is not None:
            os.close(writer)
    before = snapshot(directory)
    path = directory / (target or source)
    if fault == "permission":
        path.chmod(0)
        error = errno.EACCES
    else:
        path.unlink()
        error = errno.ENOENT
        if fault in ("missing_parent", "not_directory"):
            path.parent.rmdir()
        if fault == "not_directory":
            path.parent.write_text("file", encoding="ascii")
            error = errno.ENOTDIR
    for options in (("-u",), ("-m", "3")):
        mark = LOG.stat().st_size if LOG.exists() else 0
        check_refusal(invoke(directory, *options), source, error)
        assert snapshot(directory) == before, (kind, fault, "storage was changed")
        if LOG.exists():
            with LOG.open("rb") as log:
                log.seek(mark)
                appended = log.read()
            assert len(appended) < 65536, (kind, fault, "log grew without a bound")
            assert b"Read failure from declared source" not in appended, appended
    compiled = invoke(directory, "-c")
    assert compiled.returncode == 0, compiled.stderr
    assert snapshot(directory) == before, "compile-only changed storage"
    if fault == "permission":
        path.chmod(0o600)


def missing_after_preflight(directory, kind):
    directory.mkdir()
    writer = prepare(directory, kind, Path("source"))
    if writer is not None:
        os.close(writer)
    gate = directory / "gate"
    environment = dict(os.environ, RDB_FAULT_MODEL_INIT_GATE=str(gate))
    process = subprocess.Popen(
        [ENGINE, "plan.rql", "-k", "-f", "-u"], cwd=directory, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    try:
        deadline = time.monotonic() + 5
        while not gate.with_suffix(".ready").exists():
            assert time.monotonic() < deadline and process.poll() is None, "model gate not reached"
            time.sleep(0.01)
        (directory / "source").unlink()
        gate.with_suffix(".release").touch()
        stdout, stderr = process.communicate(timeout=5)
        check_refusal(subprocess.CompletedProcess(process.args, process.returncode, stdout, stderr), "source", errno.ENOENT)
        assert "cannot open" in stderr, stderr
        result = directory / "store/result"
        assert not result.exists() or result.stat().st_size == 0, "failed source produced records"
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate(timeout=5)


with tempfile.TemporaryDirectory(prefix="missing-source-", dir=Path.cwd()) as scratch:
    root = Path(scratch)
    faults = ["missing", "missing_parent", "not_directory", "dangling"]
    if os.geteuid() != 0:
        faults.append("permission")
    else:
        print("permission control not run: root bypasses file permissions", flush=True)
    for kind in ("TEXTFILE", "BINFILE", "DEVICE"):
        for fault in faults:
            missing_before_start(root / f"{kind}-{fault}", kind, fault)
        missing_after_preflight(root / f"{kind}-late", kind)
        print(f"{kind}: startup, -u, bounded run, compile-only and late-open checks passed", flush=True)
