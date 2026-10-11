#!/usr/bin/env python3
# Test procesu i pomocnika. Kolejnosc ExecStopPost/restart odtwarzamy jawnie:
# manager systemd nie jest dostepny w kontenerach testowych. Nie jest to test
# samego managera; powiazanie w dostarczonej jednostce sprawdza it_packaging.
import errno
import os
from pathlib import Path
import resource
import shutil
import signal
import stat
import subprocess
import sys
import time


ENGINE = Path(sys.argv[1]).resolve()
HELPER = Path(sys.argv[2]).resolve()
ROOT = Path.cwd() / "cases"
PLAN = b"STORAGE 'storage'\nSUBSTRAT 'memory'\nDECLARE a INTEGER STREAM src, 1/16 TEXTFILE 'data.txt'\nSELECT a STREAM dst FROM src\n"
MARKER = b"RDB_FAULT_TERMINATE_IN_SLOT: slot locked"
ENV = os.environ.copy()
ENV.pop("RDB_FAULT_TERMINATE_IN_SLOT", None)
FAULT_ENV = dict(ENV, RDB_FAULT_TERMINATE_IN_SLOT="1")
LOCK = Path(ENV["TMPDIR"]) / f"xretractor_service.{ENV['RDB_NAMESPACE']}.lock"


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, timeout=15, **kwargs)
    (ROOT / "last.stdout").write_bytes(result.stdout)
    (ROOT / "last.stderr").write_bytes(result.stderr)
    return result


def expect(result, code):
    assert result.returncode == code, (result.returncode, code, result.stderr.decode(errors="replace"))


def post_stop(path, code, status, result=""):
    return run(["sh", str(HELPER), str(path)],
               env=dict(ENV, EXIT_CODE=code, EXIT_STATUS=status, SERVICE_RESULT=result))


def assert_cleared(path):
    # Prawa jak po writeQueryFile silnika: 0666 bez bitow umask, nie 0600 z mktemp.
    assert path.read_bytes() == b""
    assert stat.S_IMODE(path.stat().st_mode) == 0o666 & ~UMASK, oct(path.stat().st_mode)
    assert not list(path.parent.glob("*.abort.*"))


def fixture(name):
    directory = ROOT / name
    directory.mkdir()
    (directory / "storage").mkdir()
    (directory / "data.txt").write_text("1\n2\n3\n4\n", encoding="ascii")
    query = directory / "service plan.rql"
    query.write_bytes(PLAN)
    return directory, query


def restart(directory, query):
    # Hak pozostaje wlaczony: powrot niepustego planu oznaczalby drugi SIGABRT.
    result = run([str(ENGINE), "--service", "--noanykey", "-m", "3", str(query)],
                 cwd=directory, env=FAULT_ENV)
    expect(result, 0)
    assert MARKER not in result.stderr, result.stderr
    assert query.read_bytes() == b""
    assert not LOCK.exists(), "restart zostawil blokade"


def signal_exit(directory, query, signum):
    # Bramka -x pozwala wyslac sygnal do zywego serwera bez wyscigu z koncem danych.
    with (directory / "signal.stderr").open("wb") as log:
        child = subprocess.Popen([str(ENGINE), "--service", "--noanykey", "-x", str(query)],
                                 cwd=directory, env=ENV, stdin=subprocess.DEVNULL,
                                 stdout=subprocess.DEVNULL, stderr=log)
        try:
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                assert child.poll() is None, (directory / "signal.stderr").read_bytes()
                try:
                    lock_info = LOCK.read_text().splitlines()
                    proc_status = Path(f"/proc/{child.pid}/status").read_text().splitlines()
                    caught = int(next(line.split()[1] for line in proc_status if line.startswith("SigCgt:")), 16)
                    # PID jest publikowany przed signal(SIGTERM, handleSignal).
                    # Czekamy na handler, aby nie trafic w domyslny SIGTERM.
                    if f"PID: {child.pid}" in lock_info and caught & (1 << (signal.SIGTERM - 1)):
                        break
                except FileNotFoundError:
                    pass
                time.sleep(0.02)
            else:
                raise AssertionError("serwer nie opublikowal swojego PID")
            child.send_signal(signum)
            return child.wait(timeout=15)
        finally:
            if child.poll() is None:
                child.kill()
                child.wait()


def helper_cases():
    query = ROOT / "helper plan.rql"
    for code, status, result in [("exited", "0", "success"), ("exited", "1", "exit-code"),
                                 ("exited", "5", "exit-code"), ("exited", "134", "exit-code"),
                                 ("killed", "TERM", "signal"), ("killed", "KILL", "signal"),
                                 ("dumped", "SEGV", "core-dump"), ("", "", "")]:
        query.write_bytes(PLAN)
        expect(post_stop(query, code, status, result), 0)
        assert query.read_bytes() == PLAN, (code, status, result)
    # SIGABRT po samym statusie (starszy manager nie ustawia SERVICE_RESULT), OOM po wyniku.
    for code, status, result in [("killed", "ABRT", ""), ("dumped", "ABRT", "core-dump"),
                                 ("killed", "KILL", "oom-kill")]:
        query.write_bytes(PLAN)
        result_run = post_stop(query, code, status, result)
        expect(result_run, 0)
        assert_cleared(query)
        reason = b"OOM kill" if result == "oom-kill" else b"SIGABRT"
        assert b"cleared plan after " + reason in result_run.stderr, result_run.stderr

    # Podmiana zamiast truncation: ani hardlink, ani symlink nie psuje celu.
    sentinel = ROOT / "sentinel.rql"
    for link in ["hardlink", "symlink"]:
        query.unlink()
        sentinel.write_bytes(PLAN)
        if link == "hardlink":
            os.link(sentinel, query)
        else:
            query.symlink_to(sentinel)
        expect(post_stop(query, "dumped", "ABRT"), 0)
        assert query.is_file() and not query.is_symlink()
        assert_cleared(query)
        assert sentinel.read_bytes() == PLAN

    # Blad zapisu/rename musi byc widoczny, bez falszywego sukcesu i pozostalosci.
    directory = ROOT / "directory.rql"
    directory.mkdir()
    for path in [ROOT / "missing" / "plan.rql", directory]:
        result = post_stop(path, "killed", "ABRT")
        expect(result, 1)
        assert b"cannot clear plan after SIGABRT" in result.stderr
        assert not list(ROOT.glob("*.abort.*"))
    assert not list(directory.iterdir())
    expect(post_stop("", "dumped", "ABRT"), 1)


def engine_cases():
    directory, query = fixture("terminate")
    result = run([str(ENGINE), "--service", "--noanykey", "-m", "4", "-f", str(query)],
                 cwd=directory, env=FAULT_ENV)
    expect(result, -signal.SIGABRT)
    assert MARKER in result.stderr and b"bad_alloc" in result.stderr, result.stderr
    assert query.read_bytes() == PLAN, "abort pomija atexit; plik kasuje dopiero pomocnik"
    expect(post_stop(query, "dumped", "ABRT"), 0)
    restart(directory, query)

    directory, query = fixture("direct-abort")
    assert signal_exit(directory, query, signal.SIGABRT) == -signal.SIGABRT
    assert query.read_bytes() == PLAN
    expect(post_stop(query, "killed", "ABRT"), 0)
    restart(directory, query)

    # bad_alloc poza noexcept lapie catch w executorsm::run: kod EINTR i zwykle atexit (blokada
    # zwolniona). Plik oproznia wtedy cleanup() silnika przez zatrzask, ale tylko w jednostce
    # systemd (sciezka pliku uslugi), ktorej tu nie ma; pomocnik zostawia plan.
    directory, query = fixture("bad-alloc")
    result = run([str(ENGINE), "--service", "--noanykey", "-m", "4", "-f", str(query)],
                 cwd=directory, env=dict(ENV, RDB_FAULT_BADALLOC_IN_SLOT="1"))
    expect(result, errno.EINTR)
    assert b"catch exception: std::bad_alloc" in result.stderr, result.stderr
    assert not LOCK.exists(), "bad_alloc zostawil blokade"
    expect(post_stop(query, "exited", str(errno.EINTR), "exit-code"), 0)
    assert query.read_bytes() == PLAN

    directory, query = fixture("normal-stop")
    assert signal_exit(directory, query, signal.SIGTERM) == 0
    expect(post_stop(query, "exited", "0"), 0)
    assert query.read_bytes() == PLAN

    directory, query = fixture("environment-error")
    result = run([str(ENGINE), "--service", "--noanykey", "-m", "4", "-f", str(query)],
                 cwd=directory, env=dict(ENV, RDB_FAULT_ENV_IN_SLOT="1"))
    expect(result, 5)
    expect(post_stop(query, "exited", "5"), 0)
    assert query.read_bytes() == PLAN


UMASK = os.umask(0)
os.umask(UMASK)
# SIGABRT ma pozostac sygnalem, ale test nie potrzebuje zrzutu pamieci.
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
if ROOT.exists():
    shutil.rmtree(ROOT)
ROOT.mkdir()
try:
    helper_cases()
    engine_cases()
finally:
    # Abort celowo pomija atexit. Sprzatanie musi dzialac rowniez po nieudanej asercji.
    expect(run([str(ENGINE), "--cleanup"], env=ENV), 0)
print("SIGABRT and OOM kill: plan cleared, restart idle; other exits preserve plan")
