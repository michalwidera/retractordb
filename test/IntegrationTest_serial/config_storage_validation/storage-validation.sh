#!/bin/bash
set -euo pipefail

case "${1:-}" in
  nonexistent)
    cfg="bad-storage-nonexistent.toml"
    cat > "$cfg" <<'EOF'
[storage]
dir = "./_missing_storage_dir"
EOF

    set +e
    xretractor --config "$cfg" --help >stdout.txt 2>stderr.txt
    rc=$?
    set -e

    if [ "$rc" -eq 0 ]; then
      echo "expected non-zero exit code for nonexistent storage.dir"
      exit 1
    fi
    grep -q "Configuration error: storage.dir does not exist" stderr.txt
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
    xretractor --config "$cfg" --help >stdout.txt 2>stderr.txt
    rc=$?
    set -e

    chmod 755 _storage_unwritable
    rmdir _storage_unwritable

    if [ "$rc" -eq 0 ]; then
      echo "expected non-zero exit code for unwritable storage.dir"
      exit 1
    fi
    grep -q "Configuration error: storage.dir is not writable" stderr.txt
    ;;

  *)
    echo "usage: $0 {nonexistent|unwritable}"
    exit 2
    ;;
esac
