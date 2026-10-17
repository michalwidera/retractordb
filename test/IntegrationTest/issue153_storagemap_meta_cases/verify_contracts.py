"""Diagnostyka i brak modyfikacji plikow przez xtrdb -s."""
# covers: xtrdb.command.usage~1, xtrdb.command.requires-connection~1, xtrdb.command.execute-result~1
# covers: xtrdb.command.error-reporting~1

import pathlib
import re
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

    # Dwie komendy potwierdzaja wykonanie, list/size pomijaja "ok".
    commands = subprocess.run([program], cwd=directory, text=True, capture_output=True, timeout=10,
                              input="size\nopen created { INTEGER value }\nappend\nsize\nlist 1\nquit\n")
    assert commands.returncode == 0, commands
    clean = re.sub(r"\x1b\[[0-9;]*m", "", commands.stdout)
    assert "unconnected" in clean, clean
    assert "1 Record(s)" in clean, clean
    assert len(re.findall(r"^\.ok$", clean, re.M)) == 2, clean
    assert (directory / "created").stat().st_size == 4
    refused = subprocess.run([program, "noprompt"], cwd=directory, text=True, capture_output=True, timeout=10,
                             input="open broken { INTEGER }\nsize\nhelp\nquit\n")
    assert refused.returncode == 0, refused
    assert "invalid schema" in refused.stdout and "unconnected" in refused.stdout, refused
    assert "open file [schema]" in refused.stdout, refused.stdout

    generator = pathlib.Path(__file__).resolve().with_name("generate_storage_map_fixtures.py")
    subprocess.run([sys.executable, str(generator)], cwd=directory, check=True, timeout=10)
    (directory / "plain_file.old10").write_bytes(b"123456789")
    (directory / "unbounded.desc").write_text("{ BYTE a BYTE b RETENTION 3 0 }\n")
    (directory / "unbounded_segment_0").write_bytes(bytes([1, 10, 2, 20]))
    before = {p.name: p.read_bytes() for p in directory.iterdir() if p.is_file()}
    plain = inspect("plain_file")
    retained = inspect("ret_file")
    assert plain.returncode == retained.returncode == 0, (plain, retained)
    for output in (plain.stdout, retained.stdout):
        assert re.search(r"Segments: 4\s+Records: 4", output), output
        assert "Updates: 1" in output and "Legend:" in output, output
        for marker in ("[====]", "[----]", "[~~~~]", "[XXXX]"):
            assert marker in output, (marker, output)
    for expected in ("DATA TOTAL  rec=4 src=0 seg=4", "Policy: segments=2 capacity=3", "Retention cap records: 6",
                     "Retention cap bytes: 12", "Total records: 4", "Total bytes: 8",
                     "ret_file_segment_0 rec:3 range:0-3", "ret_file_segment_1 rec:1 range:3-4"):
        assert expected in retained.stdout, (expected, retained.stdout)
    unlimited = inspect("unbounded")
    assert unlimited.returncode == 0, unlimited
    assert "Retention cap: unbounded" in unlimited.stdout, unlimited.stdout
    assert "Total records: 2" in unlimited.stdout and "Total bytes: 4" in unlimited.stdout, unlimited.stdout
    rotations = plain.stdout.split("ROTATED FILES", 1)[1]
    generations = [int(value) for value in re.findall(r"plain_file(?:\.meta|\.shadow)?\.old(\d+)", rotations)]
    assert generations == sorted(generations, reverse=True), rotations
    assert generations[0] == 10 and generations[-1] == 0, rotations
    assert re.search(r"plain_file\.old10\s+9 B", rotations), rotations
    after = {p.name: p.read_bytes() for p in directory.iterdir() if p.is_file()}
    assert after == before, (before.keys(), after.keys())
print("Storage map contracts: OK")
