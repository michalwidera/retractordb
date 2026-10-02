"""Podmiana slotCount po podlaczeniu nie zmienia granic operacji zywego serwera."""
import fcntl
import mmap
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time

xretractor, xqry = sys.argv[1:3]
namespace = os.environ["RDB_NAMESPACE"]
segment_path = Path("/dev/shm") / ("xrdbbus_v7_" + namespace)
if not segment_path.parent.is_dir():
    print("SKIP: bez /dev/shm test podmiany naglowka nie jest dostepny")
    sys.exit(77)


def client(*args, timeout=15):
    result = subprocess.run([xqry, "--server", namespace, *args],
                            capture_output=True, text=True, timeout=timeout)
    assert result.returncode == 0, (args, result.returncode, result.stdout, result.stderr)
    return result.stdout


def expect_rows(stream, value):
    output = client("-s", stream, "-m", "2")
    rows = [line.strip() for line in output.splitlines() if re.fullmatch(r"[0-9]+", line.strip())]
    assert rows == [str(value), str(value)], (stream, output)


def wait_for_stream(stream):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        output = client("-d")
        if re.search(r"\b" + re.escape(stream) + r"\b", output):
            return
        time.sleep(0.05)
    raise AssertionError(f"plan nie oglosil strumienia {stream}: {output}")


shutil.rmtree("temp", ignore_errors=True)
Path("temp").mkdir()
Path("data.txt").write_text("1\n", encoding="ascii")
base = "STORAGE 'temp'\nDECLARE a INTEGER STREAM src, 0.05 FILE 'data.txt'\n"
Path("query.rql").write_text(base + "SELECT a+1 STREAM alpha FROM src\n", encoding="ascii")
Path("reset.rql").write_text(base + "SELECT a+3 STREAM beta FROM src\n", encoding="ascii")

server = None
mapping = None
presence = None
try:
    with Path("server.log").open("wb") as log:
        server = subprocess.Popen([xretractor, "query.rql", "--name", namespace, "-r", "-k"],
                                  stdin=subprocess.DEVNULL, stdout=log, stderr=log)
    client("-w", "--hello", timeout=25)
    expect_rows("alpha", 2)
    assert server.poll() is None, server.returncode

    # Obserwator przestrzega protokolu obecnosci przez caly czas mapowania.
    presence = (Path("/tmp") / (segment_path.name + ".lock")).open("rb")
    fcntl.flock(presence, fcntl.LOCK_SH)
    held, named = os.fstat(presence.fileno()), os.stat(presence.name)
    assert (held.st_dev, held.st_ino) == (named.st_dev, named.st_ino)
    with segment_path.open("r+b") as segment:
        mapping = mmap.mmap(segment.fileno(), 0)
    # Staly prefiks v7: magic, layoutVersion, slotCount, slotSize, reserved.
    # Nie kopiujemy ukladu Slot ani zaleznego od platformy pthread_mutex_t.
    magic, version, count, slot_size, _ = struct.unpack_from("=QIIII", mapping)
    assert magic == 0x5852444242555300 and version == 7 and count == 32
    assert slot_size > 0 and len(mapping) >= 24 + count * slot_size
    struct.pack_into("=I", mapping, 12, 4096)

    # Jawny --server wskazuje odbiorce bez routingu po migawce magistrali.
    # Klient moze odmowic podlaczenia do uszkodzonego segmentu; operacje
    # wykonuje serwer, ktory podlaczyl sie przed podmiana naglowka.
    client("-a", "SELECT src[0]+2 STREAM extra FROM src")
    expect_rows("extra", 3)
    assert server.poll() is None, server.returncode
    client("--reset", "reset.rql")
    wait_for_stream("beta")
    expect_rows("beta", 4)
    directory = client("-d")
    assert not re.search(r"\b(alpha|extra)\b", directory), directory
    assert struct.unpack_from("=I", mapping, 12)[0] == 4096
    client("--hello")
    client("-k")
    assert server.wait(timeout=10) == 0, server.returncode
    print("PASS: ad-hoc, reset, wyniki i zamkniecie przy slotCount=4096")
except BaseException:
    print(f"stan serwera: {None if server is None else server.poll()}", flush=True)
    print(Path("server.log").read_text(encoding="utf-8", errors="replace"), flush=True)
    raise
finally:
    if mapping is not None:
        struct.pack_into("=I", mapping, 12, 32)
        mapping.close()
    if server is not None and server.poll() is None:
        server.terminate()
        try:
            server.wait(timeout=10)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait(timeout=5)
    if presence is not None:
        try:
            fcntl.flock(presence, fcntl.LOCK_EX | fcntl.LOCK_NB)
            held, named = os.fstat(presence.fileno()), os.stat(presence.name)
            if (held.st_dev, held.st_ino) == (named.st_dev, named.st_ino):
                segment_path.unlink(missing_ok=True)
                Path(presence.name).unlink()
        except (BlockingIOError, FileNotFoundError):
            pass
        finally:
            presence.close()
