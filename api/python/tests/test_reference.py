"""Differential acceptance against this checkout's xretractor -u (#451)."""

from __future__ import annotations

import math
import os
import re
import shutil
import struct
import subprocess
from fractions import Fraction
from pathlib import Path

import pytest

from conftest import REPO_ROOT
from test_engine import doubling_plan, write_source

CORPUS = REPO_ROOT / "test/IntegrationTest/expr_corpus"
COMMON = ("own_tail", "own_overflow", "own_deep", "rehost_integer", "rehost_double")
EXCLUDED = {name: "RULE DO DUMP requires service state" for name in (
    "own_byte", "own_uint", "own_int", "own_float", "own_double", "own_rational", "own_string", "own_mixed", "own_rules",
)}
SYNTHETIC = ("doubling", "tripled", "double_source", "nullable", "arrays", "history_average", "uint",
             "binary_nonfinite", "mean", "wrapping", "disposable", "delayed", "multirate",
             "volatile_low", "volatile_high")
# Poza wspolnym zakresem z powodem; odmowe Engine sprawdza test_engine.py.
SYNTHETIC_EXCLUDED = {"missing": "Missing-input refusals are checked separately in test_engine.py and it_untileof_stop-missing-sources (#488)"}


def plan_for(name: str, directory: Path) -> str:
    if name in COMMON:
        text = (CORPUS / f"{name}.rql").read_text()
        # Kopia uzywa aktualnej skladni; obie postacie wykonaja te same zmigrowane deklaracje.
        text = text.replace(" FILE ", " TEXTFILE ")
        text = re.sub(r"^STORAGE .*\n", "", text, flags=re.MULTILINE)
        for filename in re.findall(r"TEXTFILE '([^']+)'", text):
            source = directory / filename
            shutil.copyfile(CORPUS / filename, source)
            text = text.replace(f"'{filename}'", f"'{source}'")
        return text
    values = [1, 2, 3, 4] if name == "volatile_low" else [100, 200, 300, 400] if name == "volatile_high" else [10, 20, 30, 40, 50, 60, 70, 80]
    source = write_source(directory, values)
    if name == "nullable":
        write_source(directory, [10, "abc", 30, 40])
    if name == "arrays":
        write_source(directory, ["1 2 10", "3 4 abc", "5 6 30"])
        return f"DECLARE a INTEGER[2], b DOUBLE STREAM src, 1/2 TEXTFILE '{source}'\nSELECT * STREAM dst FROM src\n"
    if name == "history_average":
        write_source(directory, ["1 2 10", "3 4 abc", "5 6 30"])
        return f"DECLARE a INTEGER[2], b DOUBLE STREAM src, 1 TEXTFILE '{source}'\nSELECT AVG(a[0]:3) STREAM dst FROM src\n"
    if name == "uint":
        write_source(directory, [4294967295])
        return f"DECLARE a UINT STREAM src, 1 TEXTFILE '{source}'\nSELECT a STREAM dst FROM src\n"
    if name == "binary_nonfinite":
        source = directory / "values.bin"
        source.write_bytes(struct.pack("<3d", float("inf"), float("-inf"), float("nan")))
        return f"DECLARE a DOUBLE STREAM src, 1 BINFILE '{source}'\nSELECT a STREAM dst FROM src\n"
    if name == "tripled":
        return doubling_plan(source).replace("a*2 STREAM dst", "a*3 STREAM tripled").replace("1/2 TEXTFILE", "1 TEXTFILE")
    if name == "double_source":
        return doubling_plan(source).replace("a INTEGER", "a DOUBLE")
    if name == "mean":
        return f"DECLARE a INTEGER STREAM src, 1/2 TEXTFILE '{source}'\nSELECT * STREAM mean FROM AVG(src)\n"
    if name == "disposable":
        return doubling_plan(source).replace("'\nSELECT", "' DISPOSABLE\nSELECT")
    if name == "multirate":
        return (f"DECLARE a INTEGER STREAM src, 1 TEXTFILE '{source}'\n"
                f"DECLARE b INTEGER STREAM fast, 1/2 TEXTFILE '{source}'\n"
                "SELECT a STREAM slow_out FROM src\nSELECT b STREAM fast_out FROM fast\n")
    if name == "delayed":
        return doubling_plan(source).replace("FROM src", "FROM src>2")
    if name.startswith("volatile"):
        # Lustro utrwala wynik VOLATILE, ktory po wyjsciu standalone nie ma magazynu na dysku.
        return (f"DECLARE a INTEGER STREAM src, 1 TEXTFILE '{source}'\n"
                "SELECT a*2 STREAM shared FROM src VOLATILE\nSELECT * STREAM snapshot FROM shared\n")
    return doubling_plan(source)


def normalized(value):
    if isinstance(value, float) and math.isnan(value):
        return ("NaN",)
    return value


def test_corpus_scope_is_exhaustive() -> None:
    assert {path.stem for path in CORPUS.glob("*.rql")} == set(COMMON) | set(EXCLUDED)
    assert not set(SYNTHETIC) & set(SYNTHETIC_EXCLUDED)
    for name in COMMON:
        assert "DO DUMP" not in (CORPUS / f"{name}.rql").read_text()
    for name in EXCLUDED:
        assert "DO DUMP" in (CORPUS / f"{name}.rql").read_text(), EXCLUDED[name]


@pytest.mark.parametrize("name", SYNTHETIC + COMMON)
def test_embedded_matches_standalone(rdb, xretractor: Path, tmp_path: Path, name: str) -> None:
    body = plan_for(name, tmp_path)
    input_bytes = {path: path.read_bytes() for path in tmp_path.iterdir() if path.is_file()}
    budget = 20 if name == "wrapping" else None
    embedded, reference, runtime, locks = (tmp_path / part for part in ("embedded", "reference", "runtime", "locks"))
    for directory in (embedded, reference, runtime, locks):
        directory.mkdir()
    namespace = f"py451ref{os.getpid()}"
    environment = dict(os.environ, RDB_NAMESPACE=namespace, TMPDIR=str(runtime), XDG_CONFIG_HOME=str(tmp_path / "config"))
    config = tmp_path / "retractor.toml"
    config.write_text(f'[paths]\nlock_dir = "{locks}"\n')
    query = tmp_path / "query.rql"
    query.write_text(f"STORAGE '{reference}'\n" + body)
    command = [str(xretractor), "--config", str(config), "--name", namespace]
    compiled = subprocess.run([str(xretractor), "--config", str(config), "--onlycompile", "--verbose", str(query)], env=environment,
                              capture_output=True, text=True, timeout=30)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    timings = {}
    for match in re.finditer(r"^([A-Za-z_][A-Za-z_0-9]*)\(([^)]+)\)([^\n]*)", compiled.stdout, re.MULTILINE):
        tags = match[3]
        tail = re.search(r"\btail=(\d+)", tags)
        origin = re.search(r"\borigin=(\d+)", tags)
        timings[match[1]] = (Fraction(match[2]), int(tail[1]) if tail else 0, int(origin[1]) if origin else 0)
    assert timings, compiled.stdout
    # Licznik CLI zatrzymuje sie na stop_now=1; N slotow wymaga -m N+1.
    options = ["-u"] if budget is None else ["-m", str(budget + 1)]
    try:
        completed = subprocess.run(command + [str(query), "--no-clock", "--noanykey"] + options, env=environment,
                                   capture_output=True, text=True, timeout=30)
        assert completed.returncode == 0, completed.stdout + completed.stderr
    finally:
        cleanup = subprocess.run(command + ["--cleanup"], env=environment, capture_output=True, text=True, timeout=10)
        assert cleanup.returncode == 0, cleanup.stderr
    # DISPOSABLE usuwa wejscie; odtwarzamy te same bajty przed drugim wykonaniem.
    for path, data in input_bytes.items():
        path.write_bytes(data)

    with rdb.Engine(str(embedded), lock_dir=str(locks)) as engine:
        engine.compile(f"STORAGE '{embedded}'\n" + body, until_eof=budget is None)
        outputs = [stream for stream in engine.streams() if not engine.is_declared(stream)]
        assert outputs
        trace = {stream: [] for stream in outputs}
        while (budget is None or engine.slots_done < budget) and engine.step() is not None:
            for stream in outputs:
                previous = len(trace[stream])
                trace[stream].extend((index, engine.time) for index in range(previous, engine.record_count(stream)))
        for stream in outputs:
            interval, tail, origin = timings[stream]
            # Indeksy fizyczne plus origin/tail z listingu referencji ustalaja czas kazdego wyniku.
            assert trace[stream] == [(index, (index + origin + tail + 1) * interval)
                                     for index in range(engine.record_count(stream))], stream
            if stream == "shared":
                # Pierscien nie oddaje innego rekordu pod starym indeksem; zapisane lustro jest wyrocznia.
                oldest = engine.retained_from(stream)
                assert oldest > 0
                with pytest.raises(IndexError):
                    engine.record(stream, oldest - 1)
                assert tuple(engine.record(stream, oldest)) == tuple(engine.record("snapshot", oldest))
                continue
            with rdb.Storage(stream, stream, storage_param=str(reference), lock_dir=str(locks)) as stored:
                assert stored.record_count == engine.record_count(stream), stream
                assert [(field.name, field.type, field.length, field.array_count) for field in stored.descriptor] == [
                    (field.name, field.type, field.length, field.array_count) for field in engine.schema(stream)]
                actual = [tuple(normalized(value) for value in record) for record in engine.rows(stream)]
                expected = [tuple(normalized(value) for value in stored.read(index)) for index in range(stored.record_count)]
                assert actual == expected, stream
        engine.close()
    # Metadane zapisane przez obie drogi obejmuja NULL/gap i pozycje rekordow na osi strumienia.
    for stream in outputs:
        if stream != "shared":
            assert (embedded / f"{stream}.meta").read_bytes() == (reference / f"{stream}.meta").read_bytes(), stream
