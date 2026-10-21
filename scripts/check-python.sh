#!/usr/bin/env bash
# Obowiazkowa lokalna kontrola bindingu (#447), poza workflow commit.
# Osobny build z PIC nie zmienia konfiguracji ani instalacji zwyklego silnika.
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
cmake -S "$repo_dir" -B "$build_dir" -G Ninja \
  -U '_Python_*' -U nanobind_DIR \
  -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE=Debug \
  -DRDB_PYTHON=ON -DRDB_NOEXCEPT_CHECK=ON -DPython_EXECUTABLE="$python"
cmake --build "$build_dir" --parallel "$jobs" --target \
  _core xretractor xqry rdb_noexcept_check retractorcore_noexcept_check rdbembed_noexcept_check

# POST_BUILD nie odpala, gdy zmieniono tylko .py lub .pyi i _core nie jest linkowany.
cmake -E copy_directory "$repo_dir/api/python/retractordb" "$build_dir/python/retractordb"
export PYTHONPATH="$build_dir/python"
export RDB_PYTHON_MODULE_DIR="$build_dir/python"
export RDB_XRETRACTOR="$build_dir/src/retractor/xretractor"
export PATH="$build_dir/src/retractor:$build_dir/src/qry:$PATH"

"$python" - "$repo_dir" "$build_dir" <<'PY'
import shutil
import sys
import sysconfig
from pathlib import Path

import retractordb
import retractordb._core as core

repo, build = map(Path, sys.argv[1:])
package = build / "python/retractordb"
expected = package / ("_core" + sysconfig.get_config_var("EXT_SUFFIX"))
if Path(core.__file__).resolve() != expected.resolve():
    raise SystemExit(f"check-python: wrong _core: {core.__file__}; expected {expected}")
for source in (repo / "api/python/retractordb").rglob("*"):
    if source.suffix in (".py", ".pyi"):
        copied = package / source.relative_to(repo / "api/python/retractordb")
        if copied.read_bytes() != source.read_bytes():
            raise SystemExit(f"check-python: stale package copy: {copied}")
for name, directory in (("xretractor", "retractor"), ("xqry", "qry")):
    expected_binary = build / "src" / directory / name
    selected = shutil.which(name)
    if selected is None or Path(selected).resolve() != expected_binary.resolve():
        raise SystemExit(f"check-python: wrong {name}: {selected}; expected {expected_binary}")
    print(f"check-python: {name}: {selected}")
print(f"check-python: _core: {core.__file__}; Engine: {retractordb.Engine}")
print("check-python: package source comparisons passed")
PY

# Ustawienia uzytkownika nie moga ograniczyc obowiazkowego zestawu np. przez -k.
PYTEST_ADDOPTS='' "$python" -m pytest -q -ra "$repo_dir/api/python/tests" \
  --junitxml="$build_dir/python-test-results.xml"
echo "check-python: OK ($((SECONDS - started)) s); report: $build_dir/python-test-results.xml"
