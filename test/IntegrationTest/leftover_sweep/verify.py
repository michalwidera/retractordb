"""Sprzatanie pozostalosci instancji (lockFile.hpp, protokol obecnosci w bus.hpp).

Normalny koniec nie zostawia nic: ani plikow blokad, ani obiektow IPC, ani segmentu magistrali.
To, co zostawia SIGKILL, usuwa restart pod ta sama nazwa, koniec dowolnej innej instancji albo
`xretractor --cleanup` - i zaden z tych sposobow nie rusza instancji zywej.
"""
import os
from pathlib import Path
import re
import signal
import subprocess
import sys

xretractor, xqry = sys.argv[1:3]
ns = os.environ["RDB_NAMESPACE"]
tmpdir = Path(os.environ["TMPDIR"])
SHM = Path("/dev/shm")
QUERY = "DECLARE a BYTE STREAM src, 0.1 FILE '/dev/urandom'\nSELECT src[0] STREAM dst FROM src\n"

if not SHM.is_dir():
    print("SKIP IPC object checks: no /dev/shm on this platform - only lock files are checked", flush=True)


def env_for(namespace):
    return dict(os.environ, RDB_NAMESPACE=namespace)


def leftovers(name):
    """Wszystko, co instancja `name` (z przestrzenia nazw o tej samej nazwie) moze zostawic."""
    found = [p for p in (tmpdir / f"xretractor_service.{name}.lock",
                         Path("/tmp") / f"xretractor_ipc.RetractorQueryQueue.{name}.lock") if p.exists()]
    bus = re.compile(r"xrdbbus_v\d+_" + re.escape(name) + r"(\.lock)?")
    found += [p for p in Path("/tmp").iterdir() if bus.fullmatch(p.name)]
    if SHM.is_dir():
        found += [p for p in SHM.iterdir() if p.name.endswith("." + name) or bus.fullmatch(p.name)]
    return sorted(str(p) for p in found)


def start(name):
    directory = Path(name)
    directory.mkdir(exist_ok=True)
    (directory / "query.rql").write_text(QUERY, encoding="ascii")
    with (directory / "server.log").open("ab") as log:
        server = subprocess.Popen([xretractor, "query.rql", "--name", name, "-r", "-k"], cwd=directory,
                                  env=env_for(name), stdin=subprocess.DEVNULL, stdout=log, stderr=log)
    servers.append(server)
    hello = xqry_run(name, "-w", "--hello", timeout=20)
    assert hello.returncode == 0, f"{name} did not come up: {hello.stderr}"
    return server


def xqry_run(name, *args, timeout=10):
    return subprocess.run([xqry, "--server", name, *args], env=env_for(name), capture_output=True, text=True,
                          timeout=timeout)


def stop(server, name):
    xqry_run(name, "--kill")
    server.wait(timeout=15)


def kill(server):
    server.send_signal(signal.SIGKILL)
    server.wait(timeout=10)


def killed_leaving_leftovers(name):
    kill(start(name))
    left = leftovers(name)
    # Zalozenie scenariusza, nie jego teza: bez pozostalosci nie byloby czego sprzatac.
    assert left, "SIGKILL left nothing behind - the scenario lost its subject"
    return left


servers = []
dead = ns + "k"
try:
    server = start(ns)
    assert leftovers(ns), "a running server shows none of its resources - the checks would prove nothing"
    stop(server, ns)
    assert leftovers(ns) == [], f"normal exit left: {leftovers(ns)}"
    print("PASS normal exit leaves no lock files, IPC objects or bus segment", flush=True)

    killed_leaving_leftovers(dead)
    stop(start(dead), dead)
    assert leftovers(dead) == [], f"restart after SIGKILL left: {leftovers(dead)}"
    print("PASS restart after SIGKILL takes its identity back and cleans up after itself", flush=True)

    killed_leaving_leftovers(dead)
    stop(start(ns), ns)
    assert leftovers(dead) == [], f"exit of another instance did not sweep: {leftovers(dead)}"
    print("PASS exit of any instance sweeps what a killed one left behind", flush=True)

    killed_leaving_leftovers(dead)
    live = start(ns)
    live_resources = leftovers(ns)
    swept = subprocess.run([xretractor, "--cleanup"], env=env_for(ns), capture_output=True, text=True, timeout=10)
    assert swept.returncode == 0, swept.stdout + swept.stderr
    assert "Removed leftovers of dead instances" in swept.stdout, swept.stdout
    assert leftovers(dead) == [], f"--cleanup left: {leftovers(dead)}"
    assert leftovers(ns) == live_resources, f"--cleanup touched a live instance: {live_resources} -> {leftovers(ns)}"
    assert xqry_run(ns, "--hello").returncode == 0, "live instance stopped answering after --cleanup"
    stop(live, ns)
    assert leftovers(ns) == [], f"normal exit left: {leftovers(ns)}"
    print("PASS --cleanup removes what dead instances left and nothing of the live one", flush=True)
finally:
    for server in servers:
        if server.poll() is None:
            server.kill()
            server.wait()
