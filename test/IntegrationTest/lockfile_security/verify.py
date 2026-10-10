"""Bezpieczne blokady instancji: odmowy, diagnostyka oraz zwykly cykl zycia."""

import errno
import os
from pathlib import Path
import signal
import stat
import subprocess
import sys
import tempfile
import time


XRETRACTOR, CASE = sys.argv[1:3]
NAME = os.environ["RDB_NAMESPACE"]
if CASE == "foreign-uid" and os.geteuid() != 0:
    print("NIEWYKONANA WERYFIKACJA: rzeczywisty drugi UID wymaga root, chown i setuid")
    sys.exit(77)


def environment(work):
    return dict(os.environ, TMPDIR=str(work), XDG_CONFIG_HOME=str(work / "empty-config"))


def command(work):
    config = work / "config.toml"
    config.write_text(f"[paths]\nlock_dir = '{work}'\n", encoding="ascii")
    config.chmod(0o644)
    return [XRETRACTOR, "--service", "--name", NAME, "--noanykey", "--config", str(config)]


def refused(work, path, reason, **credentials):
    result = subprocess.run(command(work), cwd=work, env=environment(work),
                            stdin=subprocess.DEVNULL, capture_output=True, text=True,
                            timeout=5, **credentials)
    # Plik, ktorego nie wolno uzyc, to nie zajeta instancja: EIO, nie ENOLCK.
    assert result.returncode == errno.EIO, result
    assert str(path) in result.stderr and reason in result.stderr, result.stderr
    assert "is already running" not in result.stderr, result.stderr


def snapshot(path):
    info = path.stat()
    return info.st_ino, info.st_size, info.st_uid, path.read_bytes()


def lifecycle(base):
    for kind, reason in [("symlink", "symbolic link"), ("dangling", "symbolic link"),
                         ("hardlink", "single link"), ("fifo", "not a regular file"),
                         ("directory", "not a regular file")]:
        work = base / kind
        work.mkdir()
        path = work / f"xretractor_service.{NAME}.lock"
        target = work / "sentinel"
        before = None
        if kind in ("symlink", "hardlink"):
            target.write_bytes(b"sentinel contents\n")
            before = snapshot(target)
            if kind == "symlink":
                path.symlink_to(target)
            else:
                os.link(target, path)
        elif kind == "dangling":
            path.symlink_to(target)
        elif kind == "fifo":
            os.mkfifo(path, 0o600)
        else:
            path.mkdir()
        refused(work, path, reason)
        if before is not None:
            assert snapshot(target) == before, f"zmieniono plik kontrolny dla {kind}"
        elif kind == "dangling":
            assert path.is_symlink() and not target.exists()
        else:
            mode = path.lstat().st_mode
            assert (stat.S_ISFIFO(mode) if kind == "fifo" else stat.S_ISDIR(mode))

    work = base / "normal"
    work.mkdir()
    path = work / f"xretractor_service.{NAME}.lock"
    proc = subprocess.Popen(command(work), cwd=work, env=environment(work),
                            stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            assert proc.poll() is None, proc.communicate()
            if path.exists() and f"PID: {proc.pid}\n" in path.read_text():
                break
            time.sleep(0.01)
        else:
            raise AssertionError("brak opublikowanego PID")
        before = snapshot(path)
        competitor = subprocess.run(command(work), cwd=work, env=environment(work),
                                    stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=5)
        assert competitor.returncode == errno.ENOLCK and "is already running" in competitor.stderr, competitor
        assert snapshot(path) == before, "konkurent zmienil blokade zywego procesu"
        proc.send_signal(signal.SIGTERM)
        proc.communicate(timeout=10)
        assert proc.returncode == 0 and not path.exists()
        result = subprocess.run([*command(work), "-m", "1"], cwd=work, env=environment(work),
                                stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=10)
        assert result.returncode == 0 and not path.exists(), result
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.communicate(timeout=10)
    print("PASS: symlink, dangling, hardlink, FIFO, directory, start/publish/exclude/release/restart")


def foreign_uid(base):
    uid = 65534
    os.chown(base, uid, uid)
    path = base / f"xretractor_service.{NAME}.lock"
    path.write_bytes(b"foreign sentinel\n")
    path.chmod(0o666)
    before = snapshot(path)
    try:
        refused(base, path, "untrusted owner", user=uid, group=uid, extra_groups=[])
    except PermissionError:
        print("NIEWYKONANA WERYFIKACJA: brak uprawnien do zmiany UID procesu")
        sys.exit(77)
    assert snapshot(path) == before, "zmieniono plik blokady nalezacy do innego UID"
    print("PASS: zapis przez UID 65534 odmawia pliku UID 0 i zachowuje jego zawartosc/rozmiar")


# Drugi UID musi moc przejsc do prywatnego katalogu; katalog puli harnessu moze byc 0700.
parent = "/tmp" if CASE == "foreign-uid" else os.environ["TMPDIR"]
with tempfile.TemporaryDirectory(prefix="rdb466-", dir=parent) as directory:
    base = Path(directory)
    if CASE == "foreign-uid":
        foreign_uid(base)
    else:
        lifecycle(base)
