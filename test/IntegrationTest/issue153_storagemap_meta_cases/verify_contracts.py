"""Diagnostyka i brak modyfikacji plikow przez xtrdb -s."""

import pathlib
import struct
import subprocess
import sys
import tempfile

program = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix="map-contract-", dir=".") as sandbox:
    directory = pathlib.Path(sandbox)

    def inspect(name):
        return subprocess.run([program, "-s", name], cwd=directory, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)

    help_result = subprocess.run([program, "noprompt"], input="help\nquit\n", cwd=directory,
                                 text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
    assert help_result.returncode == 0, help_result
    for command in ("open", "read", "write", "exit"):
        assert command in help_result.stdout, (command, help_result.stdout)

    missing = inspect("missing")
    assert "Descriptor not found: missing.desc" in missing.stderr, missing
    assert not missing.stdout, missing.stdout
    descriptor = directory / "sample.desc"
    descriptor.write_text("{ INTEGER }\n")
    invalid = inspect("sample")
    assert "Descriptor does not parse: sample.desc" in invalid.stderr, invalid
    assert "Storage map:" not in invalid.stdout, invalid.stdout
    descriptor.write_text("{ INTEGER value }\n")
    absent = inspect("sample")
    assert "(missing)" in absent.stdout, absent
    (directory / "sample").write_bytes(struct.pack("i", 42))
    before = {p.name: p.read_bytes() for p in directory.iterdir() if p.is_file()}
    shown = inspect("sample")
    assert shown.returncode == 0, shown
    for section in ("DESCRIPTOR", "DATA", "META", "SHADOW", "INTEGER", "value"):
        assert section in shown.stdout, (section, shown.stdout)
    assert len(shown.stdout.splitlines()[0]) == 64, shown.stdout
    after = {p.name: p.read_bytes() for p in directory.iterdir() if p.is_file()}
    assert after == before, (before, after)
print("Storage map contracts: OK")
