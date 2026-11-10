#!/usr/bin/env python3
"""Regresje wartosci i demonstracji czterech przykladow z #612."""

import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys


ROOT = Path.cwd()
ENGINE = str(Path(sys.argv[1]).resolve())
XTRDB = str(Path(sys.argv[2]).resolve())
WORK = ROOT / "work"
shutil.rmtree(WORK, ignore_errors=True)
WORK.mkdir()


def copy_example(relative, name):
    dest = WORK / name
    shutil.copytree(ROOT / "examples" / relative, dest)
    return dest


def run(args, cwd, env=None, ok=True):
    result = subprocess.run(args, cwd=cwd, env=env, capture_output=True, text=True, timeout=30)
    with (cwd / "commands.log").open("a") as log:
        log.write(f"{args!r}\nexit={result.returncode}\n{result.stdout}{result.stderr}\n")
    if ok and result.returncode:
        raise AssertionError(f"{args!r}: exit={result.returncode}\n{result.stdout}{result.stderr}")
    return result


def execute(cwd, query, slots=30):
    run([ENGINE, "-c", "-q", query], cwd)
    run([ENGINE, "-q", query, "-f", "-m", str(slots), "-k"], cwd)


def integers(path):
    data = path.read_bytes()
    assert data and len(data) % 4 == 0, f"{path}: pusty lub niepelny rekord"
    return struct.unpack(f"<{len(data) // 4}i", data)


def expect(path, expected):
    actual = integers(path)
    reference = tuple(expected(i) for i in range(len(actual)))
    assert actual == reference, f"{path}: expected={reference[:40]}, actual={actual[:40]}"
    return len(actual)


def compare(left, right):
    # Wspolna oprawa pokazuje diff i rozmiary rowniez dla plikow binarnych.
    run(["bash", str(ROOT.parent / "compare.sh"), str(left), str(right)], WORK)


# Dwie deklaracje BINFILE, rozne tempa i wielokrotne EOF. Kontrola czyta druga
# kopie danych. Wzorzec wartosci jest niezalezny od obu przebiegow silnika.
multi = "issue-multiple-read-form-one-file.rql"
shared = copy_example("issue-set/multi-read", "shared")
separate = copy_example("issue-set/multi-read", "separate")
assert (shared / "file.dat").read_bytes() == struct.pack("<8i", *range(1, 9))
shutil.copy2(separate / "file.dat", separate / "second.dat")
plan = (separate / multi).read_text()
old = "DECLARE d INTEGER STREAM bdata,1 BINFILE 'file.dat'"
assert old in plan
(separate / multi).write_text(plan.replace(old, old.replace("file.dat", "second.dat")))
for directory in (shared, separate):
    execute(directory, multi, 2000)
    for stream in ("fast", "slow", "d2"):
        count = expect(directory / stream, lambda i: i % 8 + 1)
        assert count > 16, f"{stream}: brak pokrycia wielokrotnego EOF"
    for stream in ("d0", "d3"):
        assert expect(directory / stream, lambda i: 77) > 16
    expect(directory / "d1", lambda i: 77 if i % 6 == 0 else (i - i // 6 - 1) % 8 + 1)
for stream in ("fast", "slow", "d0", "d1", "d2", "d3"):
    for suffix in ("", ".meta"):
        compare(shared / (stream + suffix), separate / (stream + suffix))
print("multi-read: oracle i osobne kopie pliku zgodne, lacznie z metadanymi")


# Wzorce okien wyprowadzone z pozycji konca n*k i szerokosci, z faza origin.
mirror = copy_example("mwd", "mirror")
execute(mirror, "query-mwnd.rql")
expect(mirror / "agregationMirror", lambda i: (((i + 1) % 9) + 1) * 10)
compare(mirror / "agregationMirror", mirror / "serialization")
windows = copy_example("mwd", "windows")
assert (windows / "datafile3.dat").read_bytes() == struct.pack("<8i", *range(1, 9))
execute(windows, "query-mwnd2.rql")
for name, step, width in (("str1", 2, 3), ("str2", 2, -3), ("str3", 1, 2), ("str4", 1, -2)):
    size = abs(width)
    origin = math.ceil((size - 1) / step)

    def expected(i):
        record, field = divmod(i, size)
        end = (record + origin) * step
        position = end - field if width > 0 else end - size + 1 + field
        return position % 8 + 1

    assert expect(windows / name, expected) >= size * 8
print("mwd: znaki, kroki, faza okien i serializacja zgodne z wzorcami")


# DEVICE jest pokazem losowym. Wartosci wyrazen sprawdzamy dodatkowo na
# stalych bajtach; losowy przebieg musi dac niepuste, kompletne rekordy.
device = copy_example("session-record-1", "device")
execute(device, "query.rql")
for name, size in (("str1", 2), ("str2", 10)):
    data = (device / name).read_bytes()
    assert data and len(data) % size == 0, name
fixed = copy_example("session-record-1", "device-fixed")
plan = (fixed / "query.rql").read_text()
assert plan.count("DEVICE '/dev/urandom'") == 2
(fixed / "query.rql").write_text(plan.replace("DEVICE '/dev/urandom'", "BINFILE 'constant.dat'"))
(fixed / "constant.dat").write_bytes(bytes((8, 20)) * 32)
execute(fixed, "query.rql")
for name, record in (("str1", bytes((8, 20))), ("str2", struct.pack("<iBiB", 5, 8, 9, 20))):
    actual = (fixed / name).read_bytes()
    assert actual and actual == record * (len(actual) // len(record)), name
print("session-record-1: DEVICE dziala; deterministyczna kontrola wyrazen zgodna")


# Demonstracje odpalane bez terminala, spoza ich katalogu, ze spacjami w sciezce.
# Sprawdzamy faktycznie wypisane rekordy, a nie tylko kod wyjscia skryptu.
demos = copy_example("mwd", "demo with spaces")
env = os.environ | {"XRETRACTOR": ENGINE, "XTRDB": XTRDB}
originals = {p.name: p.read_bytes() for p in demos.iterdir() if p.is_file()}
for script, row in (("test-mwnd.sh", "core_0:20 core_1:30"),
                    ("test-mwnd2.sh", "core0_0:3 core0_1:2 core0_2:1")):
    output = run(["bash", str(demos / script)], WORK, env).stdout
    assert row in output, output
    for mode in ("x", "--compile-only"):
        # Sama kompilacja nie potrzebuje xtrdb i nie moze wejsc w petle wykonania.
        compile_env = env | {"XTRDB": "/nonexistent/xtrdb"}
        run(["bash", str(demos / script), "", mode], WORK, compile_env)
for query, row in (("query-mwnd3.rql", "core_0:20 core_1:30"),
                   ("query-mwnd4.rql", "core_0:10")):
    output = run(["bash", str(demos / "test-mwnd.sh"), query, "40"], WORK, env).stdout
    assert row in output, output
assert originals == {p.name: p.read_bytes() for p in demos.iterdir() if p.is_file()}
for limit in ("0", "-1", "abc"):
    result = run(["bash", str(demos / "test-mwnd.sh"), "", limit], WORK, env, ok=False)
    assert result.returncode == 2, (limit, result.returncode)
bad = WORK / "bad.rql"
bad.write_text("SELECT\n")
result = run(["bash", str(demos / "test-mwnd.sh"), str(bad)], WORK, env, ok=False)
assert result.returncode != 0, "demonstracja ukryla blad kompilacji"
print("demonstracje: wyniki, kompilacja, parametry i izolacja katalogu OK")
