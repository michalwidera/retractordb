#!/bin/bash
set -euo pipefail

# stdout/stderr per tryb (nie wspólne stdout.txt/stderr.txt): oba warianty dzielą ten sam
# WORKING_DIRECTORY i pod ctest -j mogą wykonywać się równolegle -- wspólna nazwa pliku
# powodowała nadpisanie stderr.txt przez drugi wariant przed jego własnym grepem.
#
# --status, a nie --help: pomoc działa bez walidacji konfiguracji (#426), a --status
# przechodzi przez nią i potem tylko sprawdza blokadę instancji.
mode="${1:-}"
stdout="stdout-${mode}.txt"
stderr="stderr-${mode}.txt"

case "$mode" in
  nonexistent)
    cfg="bad-storage-nonexistent.toml"
    cat > "$cfg" <<'EOF'
[storage]
dir = "./_missing_storage_dir"
EOF

    set +e
    xretractor --config "$cfg" --status >"$stdout" 2>"$stderr"
    rc=$?
    set -e

    if [ "$rc" -eq 0 ]; then
      echo "expected non-zero exit code for nonexistent storage.dir"
      exit 1
    fi
    grep -q "Configuration error: storage.dir does not exist" "$stderr"
    ;;

  unwritable)
    mkdir -p _storage_unwritable
    chmod 555 _storage_unwritable

    cfg="bad-storage-unwritable.toml"
    cat > "$cfg" <<'EOF'
[storage]
dir = "./_storage_unwritable"
EOF

    set +e
    xretractor --config "$cfg" --status >"$stdout" 2>"$stderr"
    rc=$?
    set -e

    chmod 755 _storage_unwritable
    rmdir _storage_unwritable

    if [ "$rc" -eq 0 ]; then
      echo "expected non-zero exit code for unwritable storage.dir"
      exit 1
    fi
    grep -q "Configuration error: storage.dir is not writable" "$stderr"
    ;;

  distinct-probe)
    # #430: dwa procesy na wspolnym katalogu musza wybrac rozne nazwy pliku proby zapisu.
    # Uruchomienia po kolei, nie rownolegle: nieziarnowany std::rand() dawal te sama nazwe
    # niezaleznie od czasu startu, wiec kolejnosc wystarcza do wykrycia kolizji deterministycznie.
    # Hak RDB_FAULT_KEEP_WRITE_PROBE zostawia plik proby, zeby dalo sie policzyc nazwy.
    rm -rf _storage_shared
    mkdir -p _storage_shared

    cfg="storage-distinct-probe.toml"
    cat > "$cfg" <<'EOF'
[storage]
dir = "./_storage_shared"
EOF

    for run in 1 2; do
      set +e
      RDB_FAULT_KEEP_WRITE_PROBE=1 xretractor --config "$cfg" --status >"$stdout" 2>"$stderr"
      rc=$?
      set -e
      if [ "$rc" -ne 0 ]; then
        echo "run $run: unexpected exit code $rc"
        cat "$stderr"
        exit 1
      fi
    done

    count=$(find _storage_shared -maxdepth 1 -name '.xretractor_write_probe_*.tmp' | wc -l)
    rm -rf _storage_shared
    if [ "$count" -ne 2 ]; then
      echo "expected 2 distinct probe files, found $count"
      exit 1
    fi
    ;;

  *)
    echo "usage: $0 {nonexistent|unwritable|distinct-probe}"
    exit 2
    ;;
esac
