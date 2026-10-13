#!/usr/bin/env bash
# Trwale srodowiska #453. Nie instaluje silnika ani jadra poza venv.
set -euo pipefail

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$repo_dir/scripts/notebook-versions.sh"
started=$SECONDS

usage() {
  cat <<'EOF'
Usage: scripts/jupyter-env.sh system|user [options]
  system                 pytest + nbclient, module from a compatible local build
  user                   JupyterLab and the retractordb kernel, local build or wheel
  --module-dir DIR       import root (default: build/Debug-Python/python)
  --wheel FILE           install this wheel into the user venv
  --python EXECUTABLE    Python 3.12 (default: python3.12 on PATH)
  --venv DIR             default: .venv-jupyter-PROFILE-BACKEND in the checkout
  --torch cpu|mps|cuda    default: cpu on Linux, mps on macOS arm64
                         cuda checks the Linux CUDA package without requiring a GPU
All dependencies are pinned in scripts/notebook-versions.sh. No sudo or activation
is needed. Re-running refreshes the same venv. The extension must already be built
for Python 3.12; see https://documentation.retractordb.com/system-installation/python-build.html for the build prerequisite.
EOF
}

fail() { echo "jupyter-env: $*" >&2; exit 2; }
profile=${1:-}
case "$profile" in
  -h|--help) usage; exit 0 ;;
  system|user) shift ;;
  *) usage >&2; exit 2 ;;
esac

module_dir="$repo_dir/build/Debug-Python/python"
module_requested=0
wheel=""
venv_dir=""
base_python="python$RDB_NOTEBOOK_PYTHON"
backend=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --module-dir|--wheel|--python|--venv|--torch)
      [[ $# -ge 2 && -n $2 ]] || fail "$1 needs a value"
      case "$1" in
        --module-dir) module_dir=$2; module_requested=1 ;;
        --wheel) wheel=$2 ;;
        --python) base_python=$2 ;;
        --venv) venv_dir=$2 ;;
        --torch) backend=$2 ;;
      esac
      shift 2
      ;;
    -h|--help) usage; exit 0 ;;
    *) fail "unknown option: $1" ;;
  esac
done

host=$(uname -s)
arch=$(uname -m)
case "$host/$arch" in
  Linux/x86_64|Linux/aarch64) backend=${backend:-cpu} ;;
  Darwin/arm64) backend=${backend:-mps} ;;
  *) fail "unsupported host: $host/$arch (Linux x86_64/aarch64 or macOS arm64 required)" ;;
esac
torch_args=()
case "$host/$backend" in
  Linux/cpu)
    torch_version="$RDB_NOTEBOOK_TORCH+cpu"
    torch_args=(--index-url "$RDB_NOTEBOOK_CPU_INDEX")
    ;;
  Linux/cuda)
    torch_version="$RDB_NOTEBOOK_TORCH+cu${RDB_NOTEBOOK_CUDA//./}"
    torch_args=(--index-url "$RDB_NOTEBOOK_CUDA_INDEX")
    ;;
  Darwin/mps) torch_version=$RDB_NOTEBOOK_MAC_TORCH ;;
  *) fail "torch backend '$backend' is not supported on $host" ;;
esac

if [[ -n $wheel ]]; then
  [[ $profile == user ]] || fail "the system profile requires a build-tree module"
  [[ $module_requested == 0 ]] || fail "choose either --module-dir or --wheel"
  [[ -f $wheel && $wheel == *.whl ]] || fail "no wheel: $wheel"
  wheel=$(cd "$(dirname "$wheel")" && pwd)/$(basename "$wheel")
  module_dir=""
else
  [[ -d $module_dir/retractordb ]] || fail "no package in $module_dir; build with RDB_PYTHON=ON first"
  module_dir=$(cd "$module_dir" && pwd)
  compgen -G "$module_dir/retractordb/_core*.so" >/dev/null || fail "no built _core in $module_dir"
fi

command -v "$base_python" >/dev/null || fail "Python $RDB_NOTEBOOK_PYTHON not found; use --python"
# Nie dziedziczymy importow z aktywnego srodowiska hosta.
unset PYTHONPATH PYTHONHOME RDB_PYTHON_MODULE_DIR RDB_WHEEL RDB_EXPECTED_CORE
unset PIP_TARGET PIP_PREFIX PIP_USER
export PIP_CONFIG_FILE=/dev/null
"$base_python" - "$RDB_NOTEBOOK_PYTHON" <<'PY'
import sys
if sys.version_info[:2] != tuple(map(int, sys.argv[1].split('.'))):
    sys.exit(f'jupyter-env: requires Python {sys.argv[1]}, got {sys.version.split()[0]}')
PY

venv_dir=${venv_dir:-$repo_dir/.venv-jupyter-$profile-$backend}
venv_dir=$("$base_python" - "$venv_dir" <<'PY'
from pathlib import Path
import sys
destination = Path(sys.argv[1]).resolve()
local_bin = (Path.home() / '.local/bin').resolve()
if destination == local_bin or local_bin in destination.parents:
    sys.exit('jupyter-env: the environment cannot be placed in ~/.local/bin')
print(destination)
PY
)
# Istniejacy katalog bez pyvenv.cfg nie jest naszym srodowiskiem.
if [[ -d $venv_dir && ! -f $venv_dir/pyvenv.cfg ]]; then
  fail "$venv_dir is not a venv; choose a new directory"
fi
if [[ ! -d $venv_dir ]]; then
  "$base_python" -m venv "$venv_dir"
fi
python="$venv_dir/bin/python3"
"$python" - "$RDB_NOTEBOOK_PYTHON" <<'PY'
import sys
if sys.prefix == sys.base_prefix or sys.version_info[:2] != tuple(map(int, sys.argv[1].split('.'))):
    sys.exit('jupyter-env: existing environment must be a Python ' + sys.argv[1] + ' venv')
PY

pip_args=(--disable-pip-version-check --quiet)
"$python" -m pip install "${pip_args[@]}" "${RDB_NOTEBOOK_PACKAGES[@]}"
"$python" -m pip install "${pip_args[@]}" "${torch_args[@]}" "torch==$torch_version"
if [[ $profile == user ]]; then
  "$python" -m pip install "${pip_args[@]}" "jupyterlab==$RDB_NOTEBOOK_LAB"
fi
if [[ -n $wheel ]]; then
  "$python" -m pip install "${pip_args[@]}" --no-deps "$wheel"
fi
"$python" -m pip check

# Proba importu, pochodzenia modulu i pakietu torch, takze bez urzadzenia GPU.
PYTHONPATH="$module_dir" "$python" - "$module_dir" "$backend" "$torch_version" "$RDB_NOTEBOOK_CUDA" <<'PY'
import sys
from importlib.metadata import distributions
from pathlib import Path

import numpy as np
import retractordb._core as core
import torch

root, backend, expected_torch, cuda = sys.argv[1:]
actual_core = Path(core.__file__).resolve()
expected_root = Path(root).resolve() if root else Path(sys.prefix).resolve()
if not actual_core.is_relative_to(expected_root):
    sys.exit(f'jupyter-env: wrong _core: {actual_core}, expected under {expected_root}')
assert torch.__version__ == expected_torch, (torch.__version__, expected_torch)
if backend == 'cpu':
    assert torch.version.cuda is None and torch.version.hip is None
    assert not any(d.metadata['Name'].lower().startswith('nvidia-') for d in distributions())
elif backend == 'cuda':
    assert torch.version.cuda == cuda, torch.version.cuda
else:
    assert torch.backends.mps.is_built(), 'PyPI torch must include MPS'
array = np.arange(4, dtype=np.float32)
tensor = torch.from_dlpack(array)
assert tensor.data_ptr() == array.ctypes.data
assert torch.equal(tensor + 1, torch.tensor([1., 2., 3., 4.]))
print(f'Python {sys.version.split()[0]}; numpy {np.__version__}; torch {torch.__version__}')
print(f'CUDA build: {torch.version.cuda}; GPU available: {torch.cuda.is_available()}')
print(f'retractordb._core.__file__={actual_core}')
PY

IPYTHONDIR="$venv_dir/share/ipython" "$python" -m ipykernel install --prefix "$venv_dir" \
  --name retractordb --display-name "RetractorDB ($profile, $backend)"
"$python" - "$venv_dir" "$repo_dir" "$module_dir" "$wheel" "$profile" <<'PY'
import json
import shlex
import stat
import sys
from pathlib import Path

venv, repo, module, wheel, profile = sys.argv[1:]
venv = Path(venv)
if module:
    sys.path.insert(0, module)
import retractordb._core as core

environment = {
    'PYTHONPATH': module,
    'RDB_PYTHON_MODULE_DIR': module,
    'RDB_WHEEL': wheel,
    'RDB_EXPECTED_CORE': str(Path(core.__file__).resolve()),
    'RDB_REPO_DIR': repo,
    'JUPYTER_DATA_DIR': str(venv / 'share/jupyter'),
    'JUPYTER_PATH': str(venv / 'share/jupyter'),
    'JUPYTER_CONFIG_DIR': str(venv / 'etc/jupyter'),
    'JUPYTER_RUNTIME_DIR': str(venv / 'var/run/jupyter'),
    'IPYTHONDIR': str(venv / 'share/ipython'),
    'PIP_CONFIG_FILE': '/dev/null',
    'PIP_DISABLE_PIP_VERSION_CHECK': '1',
}
kernel = venv / 'share/jupyter/kernels/retractordb/kernel.json'
spec = json.loads(kernel.read_text())
spec['env'] = environment
kernel.write_text(json.dumps(spec, indent=2) + '\n')
exports = ''.join(f'export {key}={shlex.quote(value)}\n' for key, value in environment.items())
commands = {'rdb-python': ''}
if profile == 'user':
    commands['rdb-jupyter'] = '-m jupyterlab '
for name, arguments in commands.items():
    launcher = venv / 'bin' / name
    launcher.write_text('#!/bin/sh\nunset PYTHONHOME PIP_TARGET PIP_PREFIX PIP_USER\n' + exports
                        + f'exec {shlex.quote(str(venv / "bin/python3"))} {arguments}"$@"\n')
    launcher.chmod(launcher.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
PY
"$python" -m pip freeze --all --disable-pip-version-check > "$venv_dir/requirements-resolved.txt"
printf '\nReady: %s (%s), %s seconds, %s KiB\n' "$profile" "$backend" "$((SECONDS - started))" "$(du -sk "$venv_dir" | awk '{print $1}')"
printf 'Interpreter with the selected module: %s/bin/rdb-python\n' "$venv_dir"
if [[ $profile == user ]]; then
  printf 'Open notebooks: %s/bin/rdb-jupyter %s/api/python/notebooks\n' "$venv_dir" "$repo_dir"
else
  printf 'Tests: %s/bin/rdb-python -m pytest %s/api/python/tests -ra\n' "$venv_dir" "$repo_dir"
fi
