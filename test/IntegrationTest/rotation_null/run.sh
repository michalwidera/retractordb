#!/bin/bash
set -euo pipefail

for storage in DIRECT DEFAULT POSIX POSIXSHD GENERIC; do
  rm -rf "temp-$storage"
  mkdir "temp-$storage"
  cat > query.rql <<RQL
ROTATION 'temp-$storage/counter'
STORAGE 'temp-$storage'
DECLARE a INTEGER, b INTEGER STREAM src, 1/8 TEXTFILE 'data.txt' ONESHOT
SELECT a, b STREAM result FROM src STORAGE $storage
RQL

  printf 'NULL 11\n22 NULL\n33 44\n' > data.txt
  xretractor query.rql -m 4 -f </dev/null > "server-$storage-0.txt" 2>&1 || {
    cat "server-$storage-0.txt"
    exit 1
  }
  printf '55 66\nNULL 77\n88 NULL\n' > data.txt
  xretractor query.rql -m 4 -f </dev/null > "server-$storage-1.txt" 2>&1 || {
    cat "server-$storage-1.txt"
    exit 1
  }
  python3 verify.py "temp-$storage" "$storage"
done
