#!/usr/bin/env python3
"""Regresja zamykania podgladu z zaleglymi danymi, bez Qt i globalnego IPC."""

import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


STUB = r'''#!/usr/bin/env python3
import os
from pathlib import Path
import signal
import sys
import time

state = Path(os.environ["XPLOT_TEST_STATE"])
role = Path(sys.argv[0]).name
args = sys.argv[1:]
if role == "xretractor" and "-c" in args:
    sys.exit(0)
if role == "xqry" and "-l" in args:
    if os.environ.get("XPLOT_FAIL_START"):
        sys.exit(1)
    try:
        os.kill(int((state / "xretractor.pid").read_text()), 0)
    except (FileNotFoundError, ProcessLookupError):
        sys.exit(1)
    sys.exit(0)
if role == "xqry" and "-k" in args:
    with (state / "kill-requests").open("a") as out:
        out.write("kill\n")
    os.kill(int((state / "xretractor.pid").read_text()), signal.SIGTERM)
    sys.exit(0)
(state / (role + ".pid")).write_text(str(os.getpid()))
if role == "xretractor":
    if os.environ.get("XPLOT_FAIL_START"):
        sys.exit(1)
    while True:
        signal.pause()
elif role == "xqry":
    (state / "client-stdin").write_bytes(sys.stdin.buffer.readline())
    if os.environ.get("XPLOT_FAIL_CLIENT"):
        sys.exit(1)
    # Producent nie konczy sie po serwerze: udaje zalegle kadry w kolejkach.
    while True:
        os.write(1, b"x" * 65536)
else:
    def terminated(signum, frame):
        (state / "forced-plot-stop").touch()
        sys.exit(143)
    signal.signal(signal.SIGTERM, terminated)
    (state / "binding").write_bytes(sys.stdin.buffer.readline())
    while sys.stdin.buffer.read(4096):
        (state / "rendered").touch()
        time.sleep(0.03)
'''


def await_condition(predicate, message, timeout=3):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.02)
    raise AssertionError(message)


def alive(pid):
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False


def run(source_root):
    with tempfile.TemporaryDirectory(prefix="xplot-test-") as tmp:
        root = Path(tmp)
        bindir = root / "bin"
        bindir.mkdir()
        for name in ("xretractor", "xqry", "gnuplot"):
            executable = bindir / name
            executable.write_text(STUB, encoding="ascii")
            executable.chmod(0o755)
        processes = []

        def start(name, **overrides):
            state = root / name
            state.mkdir()
            (state / "input").write_bytes(b"keyboard\n")
            env = dict(os.environ, PATH=str(bindir) + os.pathsep + os.environ["PATH"],
                       XPLOT_TEST_STATE=str(state), TMPDIR=str(state), **overrides)
            with (state / "log").open("w") as log, (state / "input").open("rb") as stdin:
                process = subprocess.Popen(
                    ["bash", str(source_root / "scripts/xplot.sh"), "stream", "query.rql", "1440,-50,350", "", name],
                    cwd=state, env=env, stdin=stdin, stdout=log, stderr=log, start_new_session=True,
                )
            processes.append(process)
            return process, state, env

        def ready(state):
            await_condition(lambda: (state / "rendered").exists(), "plot did not start: " + str(state))
            assert (state / "client-stdin").read_bytes() == b"keyboard\n", "client lost stdin"
            assert (state / "binding").read_bytes() == b'bind "Close" "exit gnuplot"\n'

        def stopped(process, state, expected=0):
            try:
                status = process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                raise AssertionError("xplot kept running after stop: " + str(state)) from None
            assert status == expected, (status, (state / "log").read_text())
            for pidfile in state.glob("*.pid"):
                pid = int(pidfile.read_text())
                assert not alive(pid), "child left behind: " + str(pidfile)
            assert not list(state.glob("xplot.*")), "temporary FIFO left behind"

        try:
            # Drugi podglad pozostaje zywy przez wszystkie scenariusze sprzatania.
            other, other_state, _ = start("other")
            ready(other_state)
            for reason in ("server", "window", "interrupt", "terminate"):
                process, state, env = start(reason)
                ready(state)
                if reason == "server":
                    subprocess.run([str(bindir / "xqry"), "-k", "--server", reason], env=env, check=True)
                elif reason == "window":
                    os.kill(int((state / "gnuplot.pid").read_text()), signal.SIGTERM)
                else:
                    process.send_signal(signal.SIGINT if reason == "interrupt" else signal.SIGTERM)
                stopped(process, state, {"interrupt": 130, "terminate": 143}.get(reason, 0))
                if reason != "window":
                    assert not (state / "forced-plot-stop").exists(), "gnuplot must close normally on EOF"
                requests = state / "kill-requests"
                assert (requests.read_text() if requests.exists() else "") == ("kill\n" if reason == "server" else ""), \
                    "cleanup sent a command by reusable server name"
                assert other.poll() is None, "another plot was stopped"
                for pidfile in other_state.glob("*.pid"):
                    assert alive(int(pidfile.read_text())), "another plot lost a child"
                print("PASS:", reason, flush=True)

            process, state, _ = start("failed-start", XPLOT_FAIL_START="1")
            stopped(process, state, 1)
            process, state, _ = start("failed-client", XPLOT_FAIL_CLIENT="1")
            stopped(process, state)
            other.terminate()
            stopped(other, other_state, 143)
            print("PASS: startup/client failure and final cleanup", flush=True)
        finally:
            # Takze stary, wadliwy skrypt nie moze zostawic procesow po czerwonym tescie.
            for process in processes:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                process.wait()


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve())
