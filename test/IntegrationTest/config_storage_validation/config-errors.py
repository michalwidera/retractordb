#!/usr/bin/env python3
"""Bledna wykryta konfiguracja zatrzymuje programy takze w Release (#426)."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile


engine, client, storage = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix="config-errors-", dir=Path.cwd()) as tmp:
    root = Path(tmp)
    config = root / "retractor" / "retractor.toml"
    config.parent.mkdir()
    env = dict(os.environ, XDG_CONFIG_HOME=str(root))

    def run(command):
        return subprocess.run(command, env=env, input="", text=True, capture_output=True, timeout=10)

    # --status xretractora przechodzi przez ladowanie i walidacje konfiguracji, a potem tylko
    # sprawdza blokade instancji. Tryb uslugowy kieruje ERROR na stderr tak samo jak przy starcie uslugi.
    engine_command = [engine, "--service", "--status"]
    missing = run(engine_command)
    assert missing.returncode == 0, missing.stderr
    assert "Configuration error" not in missing.stderr, missing.stderr

    for content, cause in (
        ('[storage\ndir = "unused"\n', "Error while parsing"),
        ('[paths]\nlock_dir = "relative/locks"\n', "must be an absolute path"),
    ):
        config.write_text(content, encoding="ascii")
        for command in (engine_command, [engine, "--service"], [engine, "--status"], [client, "--bus"], [storage, "noprompt"]):
            result = run(command)
            assert 0 < result.returncode < 128, (command, result.returncode, result.stderr)
            assert str(config) in result.stderr, (command, result.stderr)
            assert cause in result.stderr, (command, result.stderr)
            if command[0] == engine and "--service" in command:
                assert "[E]" in result.stderr, result.stderr

        # Pomoc dziala zawsze; xretractor pokazuje w niej blad konfiguracji zamiast odmowy.
        for command in ([engine, "--help"], [engine, "--service", "--help"], [client, "--help"], [storage, "--help"]):
            result = run(command)
            assert result.returncode == 0, (command, result.returncode, result.stderr)
            if command[0] == engine:
                assert "Config: error:" in result.stdout and str(config) in result.stdout, result.stdout

        # Jawne --config zachowuje ten sam kontrakt i podaje nazwe pliku.
        result = run(engine_command + ["--config", str(config)])
        assert 0 < result.returncode < 128, (result.returncode, result.stderr)
        assert "[E]" in result.stderr and str(config) in result.stderr and cause in result.stderr, result.stderr

    # Pusty lock_dir zachowuje domyslny katalog; sciezka bezwzgledna jest poprawna.
    for lock_dir in ("", str(root)):
        config.write_text(f'[paths]\nlock_dir = "{lock_dir}"\n', encoding="ascii")
        result = run(engine_command)
        assert result.returncode == 0, result.stderr

print("config errors: missing, malformed TOML, relative/absolute/empty lock_dir, explicit config, help: PASS")
