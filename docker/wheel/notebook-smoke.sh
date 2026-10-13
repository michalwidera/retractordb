#!/usr/bin/env bash
# J5: trzy notebooki na zainstalowanym kole albo lokalnym module. Wyjscia i raport
# JSON pozostaja w katalogu wynikow, poza sledzonym drzewem repozytorium.
#
# Kolo z pliku/HTTPS, w czystym kontenerze (repo tylko do odczytu):
#   docker run --rm -v "$PWD:/src:ro" -v "$PWD/build/notebook-results:/results" \
#     python:3.12-slim bash /src/docker/wheel/notebook-smoke.sh \
#     --source-commit <SHA-z-logu-budowy> --output-dir /results
# Bez argumentu wybiera jedyne kolo cp312-abi3 hosta z build/wheelhouse/.
# --without-torch odbiera brak opcjonalnego ML w osobnym, czystym srodowisku.
# Lokalny build (bez instalacji), interpreter zgodny z modulem:
#   PYTHON=.venv-jupyter-system-cpu/bin/python3 docker/wheel/notebook-smoke.sh \
#     --module-dir build/Debug-Python/python
# Prawdziwy Colab odbiera czlowiek wedlug https://documentation.retractordb.com/appendices/notebook-acceptance.html.
set -euo pipefail

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
source "$repo_dir/scripts/notebook-versions.sh"
mode=wheel
wheel=""
module_dir=""
output_dir=""
without_torch=0
source_commit=${RDB_SOURCE_COMMIT:-}
usage() {
  echo "usage: $0 [wheel-or-HTTPS-URL | --module-dir DIR] [--source-commit SHA] [--without-torch] [--output-dir DIR]" >&2
  exit 2
}
while [[ $# -gt 0 ]]; do
  case $1 in
    --module-dir|--source-commit|--output-dir)
      [[ $# -ge 2 && -n $2 ]] || usage
      case $1 in
        --module-dir) [[ -z $module_dir && -z $wheel ]] || usage; mode=module; module_dir=$2 ;;
        --source-commit) source_commit=$2 ;;
        --output-dir) output_dir=$2 ;;
      esac
      shift 2
      ;;
    --without-torch) without_torch=1; shift ;;
    -*) usage ;;
    *) [[ -z $wheel && -z $module_dir ]] || usage; wheel=$1; shift ;;
  esac
done

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/rdb-notebook-smoke.XXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT
if [[ -z $output_dir ]]; then
  output_dir=$(mktemp -d "${TMPDIR:-/tmp}/rdb-notebook-results.XXXXXX")
fi
mkdir -p "$output_dir"
output_dir=$(cd "$output_dir" && pwd)
unset RDB_EXPECTED_CORE

if [[ $mode == wheel ]]; then
  [[ $(uname -s) == Linux ]] || { echo "notebook-smoke: Linux wheels need Linux" >&2; exit 2; }
  [[ $source_commit =~ ^[0-9a-f]{40}$ ]] || {
    echo "notebook-smoke: provide --source-commit with the full SHA from the wheel build log" >&2
    exit 2
  }
  if [[ -z $wheel ]]; then
    shopt -s nullglob
    wheels=("$repo_dir"/build/wheelhouse/retractordb-*-cp312-abi3-*_"$(uname -m)".whl)
    [[ ${#wheels[@]} -eq 1 ]] || {
      echo "notebook-smoke: expected one cp312-abi3 $(uname -m) wheel in build/wheelhouse; pass a wheel or HTTPS URL" >&2
      exit 2
    }
    wheel=${wheels[0]}
  fi
  if [[ $wheel != https://* ]]; then
    [[ -f $wheel ]] || { echo "notebook-smoke: no such wheel: $wheel" >&2; exit 2; }
    wheel=$(cd "$(dirname "$wheel")" && pwd)/$(basename "$wheel")
  fi
  python=${PYTHON:-python$RDB_NOTEBOOK_PYTHON}
  "$python" -c 'import sys; sys.exit(sys.version_info[:2] != tuple(map(int, sys.argv[1].split("."))))' "$RDB_NOTEBOOK_PYTHON" || {
    echo "notebook-smoke: wheel profile requires Python $RDB_NOTEBOOK_PYTHON; set PYTHON" >&2
    exit 2
  }
  "$python" -m venv "$work_dir/venv"
  python=$work_dir/venv/bin/python3
  # Staly profil lokalnej proby, bez deklarowania wersji aktualnego Colaba.
  "$python" -m pip install --quiet --disable-pip-version-check "${RDB_NOTEBOOK_PACKAGES[@]}"
  if [[ $without_torch == 0 ]]; then
    torch_index=${RDB_TORCH_INDEX-$RDB_NOTEBOOK_CPU_INDEX}
    torch_args=()
    if [[ -n $torch_index ]]; then torch_args=(--index-url "$torch_index"); fi
    "$python" -m pip install --quiet --disable-pip-version-check "${torch_args[@]}" "torch==$RDB_NOTEBOOK_TORCH+cpu"
  fi
  export RDB_WHEEL=$wheel
  unset RDB_PYTHON_MODULE_DIR PYTHONPATH
else
  [[ -d $module_dir ]] || usage
  module_dir=$(cd "$module_dir" && pwd)
  python=${PYTHON:-python3}
  python=$("$python" -c 'import sys; print(sys.executable)')
  export RDB_PYTHON_MODULE_DIR=$module_dir PYTHONPATH=$module_dir
  unset RDB_WHEEL
fi

export RDB_REPO_DIR=$repo_dir RDB_SOURCE_COMMIT=$source_commit
export RDB_ACCEPTANCE_PATH=$output_dir/acceptance.json
context=local-$mode
if [[ -f /.dockerenv ]]; then context=container-$mode; fi
export RDB_ACCEPTANCE_CONTEXT=$context
# Wlasne specyfikacje jadra, bez kernelspec pozostawionego przez hosta.
export JUPYTER_DATA_DIR=$work_dir/share/jupyter JUPYTER_PATH=$work_dir/share/jupyter
"$python" -m ipykernel install --prefix "$work_dir" --name rdb-smoke >/dev/null

"$python" - "$repo_dir" "$work_dir" "$output_dir" "$mode" "$without_torch" <<'PY'
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path

import nbformat
from nbclient import NotebookClient

repo, work_dir, output_dir = map(Path, sys.argv[1:4])
mode, without_torch = sys.argv[4], sys.argv[5] == '1'
assert (importlib.util.find_spec('torch') is None) == without_torch, 'torch availability differs from the selected profile'


def freeze():
    listing = subprocess.run(
        [sys.executable, '-m', 'pip', 'freeze', '--all', '--disable-pip-version-check'],
        check=True, capture_output=True, text=True,
    ).stdout
    return set(listing.splitlines())


def seconds(cell):
    timing = cell.metadata['execution']
    start, end = (datetime.fromisoformat(timing[k]) for k in ('iopub.execute_input', 'shell.execute_reply'))
    return (end - start).total_seconds()


before = freeze() if mode == 'wheel' else None
report = None
for name in ('colab_quickstart', 'j0_storage', 'j1_engine'):
    notebook = repo / 'api/python/notebooks' / (name + '.ipynb')
    nb = nbformat.read(notebook, as_version=4)
    nbformat.validate(nb)
    for cell in nb.cells:
        if cell.cell_type == 'code':
            assert not re.search(r'\bDECLARE\b[^\n]*\bFILE\s', cell.source), notebook
            assert not re.search(r'\.\s*(?:min|max|avg|sumc)\b', cell.source), notebook
    try:
        NotebookClient(nb, timeout=600, kernel_name='rdb-smoke',
                       resources={'metadata': {'path': str(work_dir)}}).execute()
    finally:
        nbformat.write(nb, output_dir / (name + '.ipynb'))
    for cell in nb.cells:
        if cell.cell_type != 'code':
            continue
        print(f'--- {name}/{cell.id}: {seconds(cell):.1f} s', flush=True)
        for output in cell.outputs:
            if output.output_type == 'stream':
                print(output.text, end='', flush=True)
    if name == 'colab_quickstart':
        report = json.loads((output_dir / 'acceptance.json').read_text())
        assert report['numpy_assertion'] == 'passed'
        assert report['training']['status'] == ('skipped' if without_torch else 'passed')
        report['notebook_smoke'] = {'status': 'running', 'mode': mode, 'without_torch': without_torch, 'notebooks': {}}
        if mode == 'wheel':
            after = freeze()
            added, removed = sorted(after - before), sorted(before - after)
            if removed or [re.split(r'[ =@]', line, maxsplit=1)[0].lower() for line in added] != ['retractordb']:
                sys.exit('notebook-smoke: pip changed more than adding retractordb:\n'
                         + ''.join(f'  - {line}\n' for line in removed)
                         + ''.join(f'  + {line}\n' for line in added))
            report['notebook_smoke']['dependencies_unchanged'] = True
            report['notebook_smoke']['pip_added'] = added
            print(f"notebook-smoke: install {report['install_seconds']:.3f} s, "
                  f"import {report['import_seconds']:.3f} s; pip added only retractordb", flush=True)
        import retractordb._core as core
        expected = Path(core.__file__).resolve()
        if mode == 'module':
            assert expected.parent == Path(os.environ['RDB_PYTHON_MODULE_DIR']).resolve() / 'retractordb'
        else:
            assert expected.is_relative_to(Path(sys.prefix).resolve()), expected
            assert report['wheel']['source_commit'] == os.environ['RDB_SOURCE_COMMIT']
        print(f'notebook-smoke: selected _core={expected}', flush=True)
        os.environ['RDB_EXPECTED_CORE'] = str(expected)
    assert any(str(expected) in output.get('text', '') for cell in nb.cells for output in cell.get('outputs', []))
    report['notebook_smoke']['notebooks'][name] = {
        'status': 'passed', 'source_sha256': hashlib.sha256(notebook.read_bytes()).hexdigest(),
        'cell_seconds': {cell.id: seconds(cell) for cell in nb.cells if cell.cell_type == 'code'},
    }
    (output_dir / 'acceptance.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'notebook-smoke: {name} OK', flush=True)

report['notebook_smoke']['status'] = 'passed'
(output_dir / 'acceptance.json').write_text(json.dumps(report, indent=2) + '\n')
print(f'notebook-smoke: OK - 3/3 notebooks ({mode}, without_torch={without_torch})', flush=True)
print(f'notebook-smoke: report and executed notebooks: {output_dir}', flush=True)
PY
