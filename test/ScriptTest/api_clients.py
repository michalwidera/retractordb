"""Test obu API: prawdziwy IPC oraz kontrolowane awarie procesu xqry."""
from contextlib import contextmanager
from fractions import Fraction
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import uuid

source, xqry, xretractor, cpp, mode = sys.argv[1:]

# Klient C++ (test_api_client) jest EXCLUDE_FROM_ALL: buduje go dopiero
# `ninja test-api`. Gole `ctest` — tak testy odpala CI — trafia wiec na jego
# brak i to nie jest awaria, tylko nieodebrana opcja. 77 = SKIP_RETURN_CODE
# ustawiony w CMakeLists.txt, dzieki czemu ctest raportuje pominiecie zamiast
# bledu. `ninja test` w ogole tu nie dochodzi: filtruje etykiete `api`.
if not os.path.exists(cpp):
    print("SKIP brak " + cpp + " - zbuduj `ninja test-api`", flush=True)
    sys.exit(77)

sys.path.insert(0, str(Path(source) / "api/python"))
from retractordb import Client, Error, ReadTimeout

STUB = r'''#!/usr/bin/env python3
import json
import os
from pathlib import Path
import signal
import sys
import time

Path(os.environ['API_TEST_STATE'], 'pid.' + str(os.getpid())).touch()
args = sys.argv[1:]
stream = args[args.index('--select') + 1] if '--select' in args else 'valid'
if stream == 'wait':
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
def emit(kind, **values):
    print(json.dumps(dict(version=1, event=kind, **values)), flush=True)
schema = dict(stream=stream, delta='1/20', query='test', fields=[
    dict(name='a', type='INTEGER', count=2), dict(name='s', type='STRING', count=1),
    dict(name='r', type='RATIONAL', count=1)])
if '--hello' in args:
    emit('pong')
elif '--dir' in args:
    emit('streams', streams=[dict(name='valid', delta='1/20')])
elif '--detail' in args:
    emit('schema', **schema)
elif stream == 'badversion':
    print('{"version":2,"event":"schema"}', flush=True)
else:
    emit('schema', **schema)
    time.sleep(0.1)
    if stream == 'bad':
        print('not json', flush=True)
    elif stream == 'error':
        emit('error', code='stream_not_found', message='missing')
    elif stream == 'exit':
        sys.exit(7)
    elif stream == 'wait':
        while True: signal.pause()
    else:
        for i in range(100000 if stream == 'flood' else 3):
            os.write(2, b'diagnostic\n' * 1000)
            emit('record', stream=stream, values=['10', None, 'null\n"\\ world', '1/3'])
        emit('end', reason='limit')
'''


@contextmanager
def error(code):
    try:
        yield
    except Error as exc:
        assert exc.code == code, (exc.code, str(exc))
    else:
        raise AssertionError("Expected " + code)


def reaped(pid):
    assert not Path(f"/proc/{pid}").exists(), f"Child left behind: {pid}"


def python_fake(binary):
    with Client("test", xqry=binary, timeout=2) as db:
        assert db.ping()
        assert db.streams()[0].delta == Fraction(1, 20)
        assert db.describe("valid").fields[0].count == 2
        with db.subscribe("valid") as samples:
            pid = samples.pid
            rows = list(samples)
            assert len(rows) == 3
            assert rows[0]["a"] == [10, None]
            assert rows[0]["s"] == 'null\n"\\ world'
            assert rows[0]["r"] == Fraction(1, 3)
            assert samples.end_reason == "limit"
        reaped(pid)
        for name, code in [("badversion", "protocol_error"), ("bad", "protocol_error"),
                           ("exit", "process_exit"), ("error", "stream_not_found")]:
            with error(code):
                with db.subscribe(name) as samples:
                    samples.next(timeout=2)
        with db.subscribe("wait") as samples:
            with error("read_timeout"):
                samples.next(timeout=0.02)
            begin = time.monotonic()
            pid = samples.pid
        assert time.monotonic() - begin < 1.6
        reaped(pid)
        with error("buffer_overflow"):
            with db.subscribe("flood", capacity=2) as samples:
                time.sleep(0.3)
                samples.next(timeout=2)
        samples = db.subscribe("wait")
        pid = samples.pid
    reaped(pid)
    print("PASS Python fake", flush=True)


def real(root):
    (root / "storage").mkdir()
    name = "api_" + uuid.uuid4().hex[:12]
    env = dict(os.environ, RDB_NAMESPACE=name, TMPDIR=str(root))
    (root / "data.txt").write_text("10 11 12\n", encoding="ascii")
    (root / "query.rql").write_text("""STORAGE 'storage'
DECLARE v INTEGER[3] STREAM numbers, 1/20 FILE 'data.txt'
SELECT * STREAM copy FROM numbers
SELECT AVG(numbers[0] : 1)/3, numbers[1]/0 STREAM ratios FROM numbers
SELECT 'hello world', 'null' STREAM words FROM numbers
""", encoding="ascii")
    with (root / "server.log").open("wb") as log:
        server = subprocess.Popen([xretractor, "query.rql", "--name", name, "-k"], cwd=root,
                                  env=env, stdin=subprocess.DEVNULL, stdout=log, stderr=log)
        try:
            with Client(name, xqry=xqry, timeout=2) as db:
                deadline = time.monotonic() + 10
                while True:
                    assert server.poll() is None, (root / "server.log").read_text()
                    try:
                        db.ping()
                        break
                    except Error:
                        if time.monotonic() >= deadline:
                            raise
                        time.sleep(0.05)
                assert {s.name for s in db.streams()} >= {"numbers", "copy", "ratios", "words"}
                assert db.describe("numbers").fields[0].count == 3
                with db.subscribe("numbers", limit=3) as a, db.subscribe("copy", limit=3) as b:
                    assert a.pid != b.pid
                    for samples in (a, b):
                        rows = list(samples)
                        assert len(rows) == 3
                        for row in rows:
                            flat = []
                            for field in samples.schema.fields:
                                item = row[field.name]
                                flat.extend(item if field.count > 1 else [item])
                            assert flat == [10, 11, 12], (samples.schema, row)
                        reaped(samples.pid)
                with db.subscribe("ratios", limit=1) as samples:
                    row = samples.next(timeout=2)
                    assert row[samples.schema.fields[0].name] == Fraction(10, 3), (samples.schema, row)
                    assert row[samples.schema.fields[1].name] is None
                with db.subscribe("words", limit=1) as samples:
                    row = samples.next(timeout=2)
                    assert row[samples.schema.fields[0].name] == "hello world"
                    assert row[samples.schema.fields[1].name] == "null"
                with error("stream_not_found"):
                    db.subscribe("missing")
                with db.subscribe("numbers") as samples:
                    pid = samples.pid
                reaped(pid)
                assert db.ping()
                subprocess.run([cpp, name, xqry, "real"], cwd=root, env=env, check=True, timeout=20)
                with db.subscribe("numbers") as samples:
                    assert samples.next(timeout=2)
                    server.terminate()
                    server.wait(timeout=5)
                    deadline = time.monotonic() + 5
                    while samples.next(timeout=2) is not None:
                        assert time.monotonic() < deadline
                    assert samples.end_reason == "server_stopped_or_reloaded"
                    reaped(samples.pid)
            print("PASS Python real and server shutdown", flush=True)
        finally:
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait()


with tempfile.TemporaryDirectory(prefix="rdb-api-test-") as tmp:
    root = Path(tmp)
    if mode == "fake":
        stub = root / "fake xqry"
        stub.write_text(STUB, encoding="ascii")
        stub.chmod(0o755)
        os.environ["API_TEST_STATE"] = str(root)
        python_fake(str(stub))
        subprocess.run([cpp, "test", str(stub), "fake"], check=True, timeout=20)
        for path in root.glob("pid.*"):
            reaped(int(path.suffix[1:]))
    else:
        real(root)
