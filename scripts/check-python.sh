#!/usr/bin/env bash
# Lokalna kontrola bindingu (#447) dla buildu z RDB_PYTHON=OFF, poza workflow commit.
# Build z RDB_PYTHON=ON obejmuje ja w `ninja test` (etykieta python). Osobny build
# z PIC nie zmienia konfiguracji ani instalacji zwyklego silnika.
set -euo pipefail

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_dir="$repo_dir/build/Check-Python"
python="$repo_dir/.venv-python/bin/python3"
jobs=4

usage() {
  echo "usage: $0 [--python INTERPRETER] [--jobs N]"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --python | --jobs)
      [[ $# -ge 2 ]] || { usage >&2; exit 2; }
      case "$1" in
        --python) python=$2 ;;
        --jobs) jobs=$2 ;;
      esac
      shift 2
      ;;
    -h | --help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done
[[ $jobs =~ ^[1-9][0-9]*$ ]] || { usage >&2; exit 2; }
command -v "$python" >/dev/null || {
  echo "check-python: prepare a venv with scripts/python-venv.sh; select it with --python" >&2
  exit 2
}
python=$("$python" -c 'import sys; print(sys.executable)')
toolchain="$repo_dir/build/Debug/generators/conan_toolchain.cmake"
[[ -f $toolchain ]] || {
  echo "check-python: missing $toolchain; prepare Debug Conan dependencies first" >&2
  exit 2
}

# Brak rozszerzenia, referencji lub torch nie moze zmienic odbioru w same skipy.
"$python" - <<'PY'
import importlib
import sys

for name in ("nanobind", "numpy", "pytest", "torch"):
    try:
        module = importlib.import_module(name)
    except ImportError as exc:
        raise SystemExit(f"check-python: {sys.executable} requires {name}: {exc}") from exc
    print(f"check-python: {name} {module.__version__}")
PY

"$python" - "$repo_dir" "$build_dir" <<'PY'
import sys
from pathlib import Path

repo, build = map(Path, sys.argv[1:])
cache = build / "CMakeCache.txt"
if cache.exists():
    expected = f"CMAKE_HOME_DIRECTORY:INTERNAL={repo}"
    if expected not in cache.read_text().splitlines():
        raise SystemExit(f"check-python: {cache} belongs to another source checkout")
PY

started=$SECONDS
echo "check-python: source $(git -C "$repo_dir" rev-parse HEAD) plus working-tree changes"
echo "check-python: interpreter $python; build $build_dir; jobs $jobs"
# Konfiguracja tylko przy pierwszym uruchomieniu albo zmianie interpretera;
# po zmianie plikow CMake ninja przekonfiguruje drzewo sam.
cached=$(grep -m1 '^Python_EXECUTABLE:' "$build_dir/CMakeCache.txt" 2>/dev/null | cut -d= -f2- || true)
if [[ ! -f $build_dir/build.ninja || $cached != "$python" ]]; then
  cmake -S "$repo_dir" -B "$build_dir" -G Ninja \
    -U '_Python_*' -U nanobind_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE=Debug \
    -DRDB_PYTHON=ON -DRDB_NOEXCEPT_CHECK=ON -DPython_EXECUTABLE="$python"
fi
cmake --build "$build_dir" --parallel "$jobs" --target \
  _core xretractor xqry rdb_noexcept_check retractorcore_noexcept_check rdbembed_noexcept_check

# Kopie pakietu, srodowisko oraz pochodzenie _core, xretractor i xqry ustalaja
# testy python_stage i python_binding (test/CMakeLists.txt, conftest.py).
ctest --test-dir "$build_dir" -L python -V
echo "check-python: OK ($((SECONDS - started)) s); report: $build_dir/python-test-results.xml"
