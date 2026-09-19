"""Tozsamosc IPC: zajeta blokada zatrzymuje start, zanim cokolwiek zostanie skasowane.

1. Blokade trzyma obcy proces: start odmawia i nie dotyka magazynu.
2. Dwa prawdziwe serwery o tej samej nazwie w roznych TMPDIR i przestrzeniach magistrali. Na
   Linuksie nazwy nie sa skracane, wiec to jedyna droga do wspolnej tozsamosci IPC; ani blokada
   instancji (inny TMPDIR), ani magistrala (inny segment) tej kolizji nie widza. Drugi serwer
   odmawia startu, pierwszy nadal odpowiada, a jego obiekty IPC zostaja nietkniete.
"""
import fcntl
import os
from pathlib import Path
import re
import subprocess
import sys

xretractor, xqry = sys.argv[1:3]
name = os.environ["RDB_NAMESPACE"]
QUERY = "DECLARE a BYTE STREAM src, 0.1 FILE '/dev/urandom'\nSELECT src[0] STREAM dst FROM src\n"
PAYLOAD = b"protected payload"


def identity_lock(instance):
    return Path("/tmp") / ("xretractor_ipc.RetractorQueryQueue." + instance + ".lock")


def ipc_objects(instance):
    """Obiekty IPC instancji z ich i-wezlami; None, gdy platforma ich nie wystawia (Darwin)."""
    shm = Path("/dev/shm")
    if not shm.is_dir():
        return None
    pattern = re.compile(r"\." + re.escape(instance) + "$")
    return {p.name: p.stat().st_ino for p in shm.iterdir() if pattern.search(p.name)}


def prepare(directory):
    directory.mkdir(exist_ok=True)
    (directory / "query.rql").write_text(QUERY, encoding="ascii")
    (directory / "dst").write_bytes(PAYLOAD)
    return directory


def assert_refused(directory, env):
    result = subprocess.run(
        [xretractor, "query.rql", "--name", name, "-r", "-k", "-f", "-m", "1"],
        cwd=directory, env=env, capture_output=True, text=True, timeout=10,
    )
    assert result.returncode != 0, result.stdout + result.stderr
    assert "cannot acquire IPC identity" in result.stderr, result.stderr
    assert (directory / "dst").read_bytes() == PAYLOAD, "refused start touched the payload"


def xqry_run(*args, env, timeout=10):
    return subprocess.run([xqry, "--server", name, *args], env=env, capture_output=True, text=True, timeout=timeout)


# 1. Obcy wlasciciel blokady.
holder = prepare(Path("holder"))
lock = identity_lock(name)
with lock.open("a+") as guard:
    fcntl.flock(guard, fcntl.LOCK_EX | fcntl.LOCK_NB)
    assert_refused(holder, dict(os.environ))
    lock.unlink()  # zgodnie z protokolem: kasuje ten, kto trzyma blokade, jeszcze pod nia
print("PASS occupied IPC identity refuses startup without touching payload", flush=True)

# 2. Dwa serwery, jedna tozsamosc IPC.
first, second = prepare(Path("first")), prepare(Path("second"))
second_tmp = Path("second_tmp").resolve()
second_tmp.mkdir(exist_ok=True)
env_first = dict(os.environ)
env_second = dict(os.environ, TMPDIR=str(second_tmp), RDB_NAMESPACE=name + "b")
with (first / "server.log").open("wb") as log:
    server = subprocess.Popen([xretractor, "query.rql", "--name", name, "-r", "-k"], cwd=first, env=env_first,
                              stdin=subprocess.DEVNULL, stdout=log, stderr=log)
try:
    hello = xqry_run("-w", "--hello", env=env_first, timeout=20)
    assert hello.returncode == 0, "first server did not come up: " + hello.stderr
    before = ipc_objects(name)
    assert before is None or len(before) >= 3, f"first server has no IPC objects: {before}"

    assert_refused(second, env_second)

    assert server.poll() is None, "first server died when the second one tried to start"
    hello = xqry_run("--hello", env=env_first)
    assert hello.returncode == 0, "first server stopped answering: " + hello.stderr
    if before is None:
        print("SKIP IPC object identity check: no /dev/shm on this platform", flush=True)
    else:
        assert ipc_objects(name) == before, f"IPC objects of the first server changed: {before} -> {ipc_objects(name)}"
finally:
    xqry_run("--kill", env=env_first)
    try:
        server.wait(timeout=10)
    except subprocess.TimeoutExpired:
        server.kill()
        server.wait()
print("PASS second server with the same IPC identity refused; first one untouched and answering", flush=True)
