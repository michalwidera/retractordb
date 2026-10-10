"""Wlasnosc IPC: normalny cykl zycia i osobny scenariusz z rzeczywistym drugim UID."""

import os
from pathlib import Path
import signal
import shutil
import stat
import subprocess
import sys
import tempfile


XRETRACTOR, XQRY, CASE = sys.argv[1:4]
NAME = os.environ["RDB_NAMESPACE"]
SHM = Path("/dev/shm")
if not SHM.is_dir():
    print("NIEWYKONANA WERYFIKACJA: scenariusz wymaga linuksowego backendu /dev/shm")
    sys.exit(77)
if CASE == "foreign-uid" and os.geteuid() != 0:
    print("NIEWYKONANA WERYFIKACJA: drugi UID wymaga root, chown i zmiany UID procesow")
    sys.exit(77)

UID = 65534 if CASE == "foreign-uid" else os.geteuid()
GID = os.stat(XRETRACTOR).st_gid if CASE == "foreign-uid" else os.getegid()
AS_USER = {"user": UID, "group": GID, "extra_groups": []} if CASE == "foreign-uid" else {}
FOREIGN_UID = os.geteuid()
CREATED = set()
SERVERS = []
SUBSCRIPTIONS = set()
CRASH_QUEUES = {}


def objects(name=NAME):
    return [SHM / f"RetractorQueryQueue.{name}", SHM / f"RetractorReply_v1.{name}"]


def snapshot(path):
    info = path.stat()
    return info.st_ino, info.st_uid, stat.S_IMODE(info.st_mode), path.read_bytes()


def foreign_object(path, mode=0o666):
    fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, mode)
    CREATED.add(path)
    try:
        os.fchmod(fd, mode)
        os.write(fd, b"foreign sentinel")
    finally:
        os.close(fd)
    assert path.stat().st_uid == FOREIGN_UID != UID
    return snapshot(path)


def preserved(path, before):
    assert snapshot(path) == before, f"zmieniono obcy obiekt {path}"


def new_work(base, label):
    work = base / label
    work.mkdir()
    if AS_USER:
        os.chown(work, UID, GID)
    storage = work / "store"
    storage.mkdir()
    if AS_USER:
        os.chown(storage, UID, GID)
    config = work / "client.toml"
    config.write_text("[timing]\nserver_startup_wait_s = 5\n", encoding="ascii")
    config.chmod(0o644)
    return work


def environment(work):
    return dict(os.environ, TMPDIR=str(work))


def query(work, suffix=""):
    path = work / "query.rql"
    (work / "data.txt").write_text("1\n2\n3\n4\n", encoding="ascii")
    (work / "data.txt").chmod(0o644)
    path.write_text(f"STORAGE 'store'\nDECLARE a BYTE STREAM src{suffix}, 1/100 TEXTFILE 'data.txt'\n"
                    f"SELECT src{suffix}[0] STREAM dst{suffix} FROM src{suffix}\n", encoding="ascii")
    path.chmod(0o644)
    return path


def client(work, *args, name=NAME, check=True):
    command = [XQRY, "--server", name, "--config", str(work / "client.toml"), *args]
    with subprocess.Popen(command, cwd=work, env=environment(work), stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True, **AS_USER) as proc:
        if "-s" in args:
            SUBSCRIPTIONS.add(SHM / f"brcdbr.{name}.{proc.pid}")
        try:
            output, errors = proc.communicate(timeout=15)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.communicate()
            raise
        result = subprocess.CompletedProcess(command, proc.returncode, output, errors)
    if check:
        assert result.returncode == 0, (args, result.returncode, result.stdout, result.stderr)
    return result


def server(work, name=NAME, suffix="", ready=True):
    log = work / "process.log"
    with log.open("wb") as output:
        proc = subprocess.Popen([XRETRACTOR, str(query(work, suffix)), "--name", name, "--noanykey"],
                                cwd=work, env=environment(work), stdin=subprocess.DEVNULL,
                                stdout=output, stderr=subprocess.STDOUT, **AS_USER)
    SERVERS.append(proc)
    if ready:
        client(work, "-l", "-w", name=name)
        assert proc.poll() is None, log.read_text()
    return proc


def stop(proc, sig=signal.SIGTERM):
    proc.send_signal(sig)
    code = proc.wait(timeout=10)
    assert code == (0 if sig == signal.SIGTERM else -signal.SIGKILL), code


def diagnostic(work, path):
    text = "\n".join(f.read_text(errors="replace") for f in work.glob("*.log"))
    assert path.name in text and f"owner UID {FOREIGN_UID}" in text and "untrusted owner" in text, text


def lifecycle(base):
    first = new_work(base, "first")
    proc = server(first)
    bus_path = SHM / f"xrdbbus_v7_{NAME}"
    for path in [*objects(), bus_path]:
        info = path.stat()
        assert info.st_uid == UID and stat.S_IMODE(info.st_mode) == 0o600, path
    client(first, "-s", "dst", "-m", "2")
    old_inodes = [path.stat().st_ino for path in objects()]
    stop(proc, signal.SIGKILL)
    # SIGKILL omija destruktor serwera. Zachowujemy do sprzatania wylacznie
    # kolejki naszych odebranych klientow; nowe instancje nie znaja ich PID.
    for path in SUBSCRIPTIONS:
        if path.exists():
            before = snapshot(path)
            assert before[1] == UID
            CRASH_QUEUES[path] = before
    assert all(path.exists() for path in objects()), "brak pozostalosci po awarii"

    restarted = server(first)
    assert [path.stat().st_ino for path in objects()] != old_inodes, "restart przyjal stare obiekty"
    client(first, "-s", "dst", "-m", "2")
    second_name = NAME + "-b"
    second = new_work(base, "second")
    peer = server(second, second_name, "b")
    peer_inodes = [path.stat().st_ino for path in objects(second_name)]
    stop(restarted)
    assert not any(path.exists() for path in objects()), "wlasne obiekty nie zostaly posprzatane"
    assert [path.stat().st_ino for path in objects(second_name)] == peer_inodes
    client(second, "-s", "dstb", "-m", "2", name=second_name)
    stop(peer)
    assert not any(path.exists() for path in objects(second_name))
    print("PASS: 0600, start, subskrypcja, restart po SIGKILL, rozdzielenie i sprzatanie instancji")


def foreign_uid(base):
    # Serwer dziala jako UID 65534; obiekty podklada root (UID 0). Takze klient
    # oczekuje 65534 na podstawie wlasnych uprawnien, nie metadanych segmentu.
    for index, path in enumerate(objects()):
        for mode in (0o666, 0o600):
            work = new_work(base, f"startup-{index}-{mode}")
            before = foreign_object(path, mode)
            proc = server(work, ready=False)
            assert proc.wait(timeout=10) != 0, "start przyjal obcy obiekt"
            diagnostic(work, path)
            preserved(path, before)
            path.unlink()
            CREATED.remove(path)

    work = new_work(base, "client-command")
    proc = server(work)
    stop(proc, signal.SIGKILL)
    command, response = objects()
    # Wlasny poprawny segment odpowiedzi pozostaje, kolejka komend jest obca.
    command.unlink()
    before = foreign_object(command)
    result = client(work, "-l", check=False)
    assert result.returncode != 0
    diagnostic(work, command)
    preserved(command, before)
    command.unlink()
    CREATED.remove(command)
    response.unlink()

    work = new_work(base, "client-response")
    before = foreign_object(response)
    result = client(work, "-l", check=False)
    assert result.returncode != 0
    diagnostic(work, response)
    preserved(response, before)
    response.unlink()
    CREATED.remove(response)

    work = new_work(base, "bus")
    bus_path = SHM / f"xrdbbus_v7_{NAME}"
    # Po SIGKILL moze zostac wlasny segment. Przed usunieciem jego wlasciciel
    # jest znany i wszystkie uruchomione procesy zostaly juz odebrane wait().
    if bus_path.exists():
        assert bus_path.stat().st_uid == UID
        bus_path.unlink()
    before = foreign_object(bus_path)
    proc = server(work)
    diagnostic(work, bus_path)
    preserved(bus_path, before)
    stop(proc)
    preserved(bus_path, before)
    bus_path.unlink()
    CREATED.remove(bus_path)

    work = new_work(base, "subscription")
    proc = server(work)
    # Potomek czeka na potoku, aby obca kolejka powstala pod jego PID PRZED show.
    read_fd, write_fd = os.pipe()
    launcher = "import os,sys; os.read(int(sys.argv[1]),1); os.execv(sys.argv[2],sys.argv[2:])"
    subscriber = subprocess.Popen([sys.executable, "-c", launcher, str(read_fd), XQRY, "--server", NAME,
                                   "-s", "dst", "-m", "1"], cwd=work, env=environment(work),
                                  pass_fds=(read_fd,), stdout=subprocess.PIPE, stderr=subprocess.PIPE, **AS_USER)
    os.close(read_fd)
    path = SHM / f"brcdbr.{NAME}.{subscriber.pid}"
    try:
        before = foreign_object(path)
        os.write(write_fd, b"x")
        output = subscriber.communicate(timeout=15)
        assert subscriber.returncode != 0, output
        diagnostic(work, path)
        preserved(path, before)
        client(work, "-l")
    finally:
        os.close(write_fd)
        if subscriber.poll() is None:
            subscriber.kill()
            subscriber.wait()
    stop(proc)
    preserved(path, before)
    path.unlink()
    CREATED.remove(path)
    print("PASS: rzeczywiste UID 0/65534, odmowa serwera i klienta, segmentow, kolejki komend i subskrypcji")


with tempfile.TemporaryDirectory(prefix="rdb465-", dir=os.environ["TMPDIR"]) as directory:
    base = Path(directory)
    if AS_USER:
        os.chown(base, UID, GID)
    try:
        if CASE == "lifecycle":
            lifecycle(base)
        else:
            foreign_uid(base)
    except BaseException:
        evidence = Path.cwd() / "evidence" / CASE
        evidence.mkdir(parents=True, exist_ok=True)
        for log in base.rglob("*.log"):
            shutil.copy2(log, evidence / (log.parent.name + "-" + log.name))
        print(f"Diagnostyka zachowana w {evidence}", flush=True)
        raise
    finally:
        for proc in SERVERS:
            if proc.poll() is None:
                proc.kill()
                proc.wait(timeout=10)
        for path in CREATED:
            path.unlink(missing_ok=True)
        for path, before in CRASH_QUEUES.items():
            if path.exists():
                preserved(path, before)
                path.unlink()
