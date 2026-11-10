#!/usr/bin/env bash
# Ten sam pokaz wsadowy, domyslnie dla czterech wariantow znaku/kroku okna.
# Uzycie: test-mwnd2.sh [plik.rql] [liczba-iteracji | x | --compile-only]
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
exec bash "$script_dir/test-mwnd.sh" "${1:-$script_dir/query-mwnd2.rql}" "${@:2}"
